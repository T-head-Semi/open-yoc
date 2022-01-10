/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/vol_scale.h"
#include "output/ao.h"
#include "output/mixer.h"

#define TAG    "ao"

//#define AO_DEBUG
#ifdef AO_DEBUG
char *g_dump_data;
int g_dump_size;
#define AO_DUMP_SIZE (1*1024*1024)
#endif

static struct {
    int                     cnt;
    const struct ao_ops     *ops[AO_OPS_MAX];
    struct {
        int                 open_ref;
        int                 start_ref;
        ao_cls_t            *ao_real;
        void                *ao_priv;
#if CONFIG_AO_MIXER_SUPPORT
#define MIX_BUF_SIZE        (2048)
        mixer_t             *mixer;
        uint8_t             mbuf[MIX_BUF_SIZE];
#endif
        aos_mutex_t         lock;
    } refs;
#define ao_ref_lock()       (aos_mutex_lock(&g_aoers.refs.lock, AOS_WAIT_FOREVER))
#define ao_ref_unlock()     (aos_mutex_unlock(&g_aoers.refs.lock))

#define ao_ref_open_get()   (g_aoers.refs.open_ref)
#define ao_ref_open_inc()   (g_aoers.refs.open_ref++)
#define ao_ref_open_dec()   (g_aoers.refs.open_ref--)

#define ao_ref_start_get()  (g_aoers.refs.start_ref)
#define ao_ref_start_inc()  (g_aoers.refs.start_ref++)
#define ao_ref_start_dec()  (g_aoers.refs.start_ref--)

#define ao_get_real()       (g_aoers.refs.ao_real)
#define ao_set_real(p)      (g_aoers.refs.ao_real = (p))

#define ao_get_priv()       (g_aoers.refs.ao_priv)
#define ao_set_priv(p)      (g_aoers.refs.ao_priv = (p))

#define ao_get_mixer()      (g_aoers.refs.mixer)
#define ao_set_mixer(p)     (g_aoers.refs.mixer = (p))

#define ao_get_mixer_buf()  (&g_aoers.refs.mbuf[0])

#define ao_ref_dump()       do { LOGD(TAG, "ao ref: openref = %2d, startref = %2d, fun = %s", ao_ref_open_get(), ao_ref_start_get(), __FUNCTION__); } while(0)
} g_aoers;

static int _ao_filters_open2(ao_cls_t *o);
static int _pcm_filter(ao_cls_t *o, const uint8_t *pcm, size_t pcm_size, uint8_t **dst, size_t *size);

static int _ao_init()
{
    static int inited = 0;

    if (!inited) {
        aos_mutex_new(&g_aoers.refs.lock);
        inited = 1;
    }

    return inited ? 0 : -1;
}

static const struct ao_ops* _get_ao_ops_by_name(const char* name)
{
    int i;

    for (i = 0; i < g_aoers.cnt; i++) {
        if (strcmp(g_aoers.ops[i]->name, name) == 0) {
            return g_aoers.ops[i];
        }
    }

    LOGE(TAG, "get ao ops err, name = %s", name);

    return NULL;
}

/*
ao_evet == 0, ao stop
ao_evt == 1, ao start
*/
__attribute__((weak)) void ao_event_hook(int ao_evt)
{
    return;
}

#if CONFIG_AO_MIXER_SUPPORT
static int __ao_open(ao_cls_t *ao, sf_t sf, const ao_conf_t *ao_cnf)
{
    int rc;
    sf_t osf;
    ao_cls_t *o = NULL;
    avframe_t *frame = NULL;

    UNUSED(ao);
    ao_ref_lock();
    if (ao_ref_open_get() == 0) {
        o = av_zalloc(sizeof(ao_cls_t));
        CHECK_RET_TAG_WITH_GOTO(o, err);

        if (ao_cnf->eq_segments) {
            o->eq_params = av_zalloc(sizeof(eqfp_t) * ao_cnf->eq_segments);
            CHECK_RET_TAG_WITH_GOTO(o->eq_params, err);
        }
        if (ao_cnf->aef_conf && ao_cnf->aef_conf_size) {
            o->aef_conf = av_malloc(ao_cnf->aef_conf_size);
            CHECK_RET_TAG_WITH_GOTO(o->aef_conf, err);
            memcpy(o->aef_conf, ao_cnf->aef_conf, ao_cnf->aef_conf_size);
            o->aef_conf_size = ao_cnf->aef_conf_size;
        }

        if (ao_cnf->resample_rate) {
            osf = sf_make_channel(CONFIG_AV_AO_CHANNEL_NUM) | sf_make_rate(ao_cnf->resample_rate) | sf_make_bit(16) | sf_make_signed(1);
        } else {
            osf = sf_make_channel(CONFIG_AV_AO_CHANNEL_NUM) | sf_make_rate(sf_get_rate(sf)) | sf_make_bit(16) | sf_make_signed(1);
        }

        frame = avframe_alloc();
        CHECK_RET_TAG_WITH_GOTO(frame, err);

        o->ops = _get_ao_ops_by_name(ao_cnf->name);
        if (!o->ops) {
            LOGE(TAG, "ao ops get failed, name = %s\n", ao_cnf->name);
            goto err;
        }

        //FIXME: vol & atempo don't work after mixer
        o->vol_en         = 0;
        o->atempo_play_en = 0;

        o->oframe         = frame;
        o->ori_sf         = sf;
        o->resample_rate  = ao_cnf->resample_rate;
        o->eq_segments    = ao_cnf->eq_segments;
        o->period_ms      = ao_cnf->period_ms ? ao_cnf->period_ms : AO_ONE_PERIOD_MS;
        o->period_num     = ao_cnf->period_num ? ao_cnf->period_num : AO_TOTAL_PERIOD_NUM;
        o->speed          = 1;
        o->sf             = osf;

        rc = _ao_filters_open2(o);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

        rc = o->ops->open(o, sf);
        if (rc < 0) {
            LOGE(TAG, "ao open failed\n");
            goto err;
        }
        ao_set_real(o);
    }
    o = ao_get_real();
    ao_ref_open_inc();
    ao_ref_dump();
    ao_ref_unlock();

    return 0;
err:
    ao_ref_unlock();
    if (o) {
        av_free(o->eq_params);
        av_free(o->aef_conf);
        av_free(o);
    }
    avframe_free(&frame);
    return -1;
}

static int __ao_control(int cmd, void *arg, size_t *arg_size)
{
    int rc = -1;
    size_t size;
    ao_cls_t *o;

    CHECK_PARAM(arg, -1);
    ao_ref_lock();
    o = ao_get_real();

    switch (cmd) {
    case AO_CMD_EQ_ENABLE: {
        oeq_seten_t *val = arg;

        if (o->avf_eq) {
            eq_seten_t para;

            size = sizeof(para);
            para.enable = val->enable;
            rc = avf_control(o->avf_eq, AVF_CMD_EQ_SET_ENABLE, (void*)&para, &size);
        } else {
            rc = 0;
        }
        o->eq_en = (rc == 0) ? val->enable : o->eq_en;
        break;
    }
    case AO_CMD_EQ_SET_PARAM: {
        oeq_setpa_t *val = arg;

        if (!((val->segid < o->eq_segments) && o->eq_params)) {
            LOGE(TAG, "param faild. segid = %u, eq_segments = %u", val->segid, o->eq_segments);
            break;
        }

        if (o->avf_eq) {
            eq_setpa_t para;
            size       = sizeof(para);
            para.segid = val->segid;
            memcpy(&para.param, &val->param, sizeof(eqfp_t));
            rc = avf_control(o->avf_eq, AVF_CMD_EQ_SET_PARAM, (void*)&para, &size);
        } else {
            rc = 0;
        }
        if (rc == 0) {
            memcpy(&o->eq_params[val->segid], &val->param, sizeof(eqfp_t));
        }
        break;
    }
    default:
        break;
    }
    ao_ref_unlock();

    return rc;
}
#else
static int __ao_open(ao_cls_t *ao, sf_t sf, const ao_conf_t *ao_cnf)
{
    int rc;

    ao_ref_lock();
    if (ao_ref_open_get() == 0) {
        ao_cls_t *o = ao;

        rc = o->ops->open(o, sf);
        if (rc < 0) {
            LOGE(TAG, "ao open failed\n");
            goto err;
        }
        ao_set_real(o);
        ao_set_priv(o->priv);
    } else {
        ao->priv = ao_get_priv();
    }
    ao_ref_open_inc();
    ao_ref_dump();
    ao_ref_unlock();

    return 0;
err:
    ao_ref_unlock();
    return -1;
}
#endif

static int __ao_start(ao_cls_t *ao)
{
    int rc = 0;

    ao_ref_lock();
#if CONFIG_AO_MIXER_SUPPORT
    mixer_set_cnl_status(ao_get_mixer(), ao->cnl, MIXER_CNL_STATUS_RUNING);
#endif
    if (ao_ref_start_get() == 0) {
#if CONFIG_AO_MIXER_SUPPORT
        UNUSED(ao);
        ao_cls_t *o = ao_get_real();
#else
        ao_cls_t *o = ao;
#endif
        ao_event_hook(1);
        rc = o->ops->start(o);
    }
    ao_ref_start_inc();
    ao_ref_dump();
#if XIAOYA_PATCH
#if CONFIG_AO_MIXER_SUPPORT
    if (mixer_get_nb_cnls(ao_get_mixer()) > 1) {
        avf_set_bypass(ao_get_real()->avf_aef, 1);
    } else {
        avf_set_bypass(ao_get_real()->avf_aef, ao->speed == 1 ? 0 : 1);
    }
#else
    avf_set_bypass(ao->avf_aef, ao->speed == 1 ? 0 : 1);
#endif
#endif
    ao_ref_unlock();

    return rc;
}

static int __ao_stop(ao_cls_t *ao)
{
    int rc = 0;

    ao_ref_lock();
#if CONFIG_AO_MIXER_SUPPORT
    mixer_set_cnl_status(ao_get_mixer(), ao->cnl, MIXER_CNL_STATUS_PAUSED);
#endif
    ao_ref_start_dec();
    if (ao_ref_start_get() == 0) {
#if CONFIG_AO_MIXER_SUPPORT
        UNUSED(ao);
        ao_cls_t *o = ao_get_real();
#else
        ao_cls_t *o = ao;
#endif

        rc = o->ops->stop(o);
        ao_event_hook(0);
    }
    ao_ref_dump();
    ao_ref_unlock();

    return rc;
}

#if defined(CONFIG_AO_DIFF_SUPPORT) && CONFIG_AO_DIFF_SUPPORT
static int g_ao_diff_en = 1;
void av_ao_diff_enable(int en)
{
    g_ao_diff_en = en;
}

static int __ao_diff_proc(const uint8_t *pcm, size_t pcm_size)
{
    int i = 0;
    if (g_ao_diff_en == 0) {
        return -1;
    }
    //printf("%x\n", pcm);
    pcm_size = pcm_size / 4 * 4;
    for(i = 0; i < pcm_size; i+= 4) {
        int16_t * left = (int16_t *)&pcm[i];
        int16_t * right = (int16_t *)&pcm[i + 2];
        *right = -(*left);
    }

    return 0;
}
#endif

static int __ao_write(ao_cls_t *ao, const uint8_t *buf, size_t count)
{
    int rc = 0;

#if CONFIG_AO_MIXER_SUPPORT
    ao_ref_lock();
    if (ao_ref_start_get() > 0) {
        uint8_t *pcm;
        size_t pcm_size;
        ao_cls_t *o = ao_get_real();

        UNUSED(ao);
        rc = _pcm_filter(o, buf, count, &pcm, &pcm_size);
        if (rc < 0) {
            LOGE(TAG, "pcm convert fail, rc = %d", rc);
            goto quit;
        }
        if (!(pcm && pcm_size)) {
            //FIXME: inner filter need more data
            goto quit;
        }
#if defined(CONFIG_AO_DIFF_SUPPORT) && CONFIG_AO_DIFF_SUPPORT
        __ao_diff_proc(pcm, pcm_size);
#endif
        rc = o->ops->write(o, pcm, pcm_size);
    }
quit:
    ao_ref_unlock();
#else
    ao_ref_lock();
    if (ao_ref_start_get() > 0) {
        ao_cls_t *o = ao;
#if defined(CONFIG_AO_DIFF_SUPPORT) && CONFIG_AO_DIFF_SUPPORT
        __ao_diff_proc(buf, count);
#endif
        rc = o->ops->write(o, buf, count);
    }
    ao_ref_unlock();
#endif

    return rc;
}

static int __ao_drain(ao_cls_t *ao)
{
    int rc = 0;

#if CONFIG_AO_MIXER_SUPPORT
    {
        int ms, size;

        ao_ref_lock();
        ms    = ao->period_ms * ao->period_num;
        size  = mixer_cnl_get_size(ao->cnl) + MIX_BUF_SIZE;
        ms   += size * 8 * 1000 / sf_get_bps(mixer_get_sf(ao_get_mixer())) + 60;
        ao_ref_unlock();
        //FIXME: sleep simple, not accurate
        aos_msleep(ms);
    }
#else
    ao_ref_lock();
    if (ao_ref_start_get() > 0) {
        if (ao->ops->drain)
            rc = ao->ops->drain(ao);
    }
    ao_ref_dump();
    ao_ref_unlock();
#endif

    return rc;
}

static int __ao_close(ao_cls_t *ao)
{
    int rc = 0;

    ao_ref_lock();
    ao_ref_open_dec();
    if (ao_ref_open_get() == 0) {
#if CONFIG_AO_MIXER_SUPPORT
        UNUSED(ao);
        ao_cls_t *o = ao_get_real();

        rc = o->ops->close(o);
        ao_set_real(NULL);

        avf_chain_close(o->avfc);
        avframe_free(&o->oframe);
        av_free(o->eq_params);
        av_free(o->aef_conf);
        av_free(o);
#else
        ao_cls_t *o = ao;

        rc = o->ops->close(o);
        ao_set_real(NULL);
#endif
    }
    ao_ref_dump();
    ao_ref_unlock();

    return rc;
}

#if CONFIG_AO_MIXER_SUPPORT
static void _mixer_ao_task(void *arg)
{
    int rc;
    uint8_t *buf = ao_get_mixer_buf();

    for (;;) {
        rc = mixer_read(ao_get_mixer(), buf, MIX_BUF_SIZE, 2000);
        if (rc > 0) {
            __ao_write(ao_get_real(), buf, rc);
        }
    }
}

#endif

/**
 * @brief  regist ao ops
 * @param  [in] ops
 * @return 0/-1
 */
int ao_ops_register(const struct ao_ops *ops)
{
    int i;

    if (ops && (g_aoers.cnt < AO_OPS_MAX)) {
        for (i = 0; i < g_aoers.cnt; i++) {
            if (strcmp(ops->name, g_aoers.ops[i]->name) == 0) {
                /* replicate  */
                break;
            }
        }

        if (i == g_aoers.cnt) {
            g_aoers.ops[g_aoers.cnt] = ops;
            g_aoers.cnt++;
        }
        return 0;
    }

    LOGE(TAG, "ao ops regist fail\n");
    return -1;
}

/**
 * @brief  init the ao config param
 * @param  [in] ao_cnf
 * @return 0/-1
 */
int ao_conf_init(ao_conf_t *ao_cnf)
{
    CHECK_PARAM(ao_cnf, -1);
    memset(ao_cnf, 0, sizeof(ao_conf_t));
    ao_cnf->name       = "alsa";
    ao_cnf->speed      = 1;
    ao_cnf->period_ms  = AO_ONE_PERIOD_MS;
    ao_cnf->period_num = AO_TOTAL_PERIOD_NUM;
    ao_cnf->db_min     = VOL_SCALE_DB_MIN;
    ao_cnf->db_max     = VOL_SCALE_DB_MAX;

    return 0;
}

static avfilter_t* _ao_eqx_new(ao_cls_t *o, sf_t eq_sf)
{
    int rc;
    int i;
    size_t size;
    eq_avfp_t eqp;
    eq_seten_t eqen;
    eq_setpa_t eqpa;
    avfilter_t *avf_eq;

    eqp.sf          = eq_sf;
    eqp.eq_segments = o->eq_segments;
    avf_eq = avf_eq_open("eq", &eqp);
    CHECK_RET_TAG_WITH_RET(avf_eq, NULL);

    size = sizeof(eq_setpa_t);
    for (i = 0; i < o->eq_segments; i++) {
        if (o->eq_params[i].type != EQF_TYPE_UNKNOWN) {
            eqpa.segid = i;
            memcpy(&eqpa.param, (void*)&o->eq_params[i], sizeof(eqfp_t));
            rc = avf_control(avf_eq, AVF_CMD_EQ_SET_PARAM, (void*)&eqpa, &size);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        }
    }
    size        = sizeof(eq_seten_t);
    eqen.enable = o->eq_en;
    rc          = avf_control(avf_eq, AVF_CMD_EQ_SET_ENABLE, (void*)&eqen, &size);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    return avf_eq;
err:
    avf_close(avf_eq);
    return NULL;
}

static int _ao_filters_open1(ao_cls_t *o)
{
    swr_avfp_t swrp;
    sf_t s16sf, osf      = o->sf;
    avfilter_t *avfc     = NULL;
    avfilter_t *avf_swr1 = NULL, *avf_swr2 = NULL;
    avfilter_t *avf_vol  = NULL, *avf_atempo = NULL;

    if (o->vol_en || o->atempo_play_en) {
        s16sf = sf_make_channel(1) | sf_make_rate(sf_get_rate(o->ori_sf)) | sf_make_bit(16) | sf_make_signed(1);
        if (s16sf != o->ori_sf) {
            /* swr first */
            swrp.osf = s16sf;
            swrp.isf = o->ori_sf;
            avf_swr1 = avf_swr_open("swr1", &swrp);
            CHECK_RET_TAG_WITH_GOTO(avf_swr1, err);
            avfc = avfc ? avfc : avf_swr1;
        }

        if (o->atempo_play_en) {
            atempo_avfp_t atempop;

            memset(&atempop, 0, sizeof(atempo_avfp_t));
            atempop.sf    = s16sf;
            atempop.speed = o->speed;
#if ATEMPO_FREQ_REDUCE
            avf_atempo = avf_atempo2_open("atempo2", &atempop);
#else
            avf_atempo = avf_atempo_open("atempo", &atempop);
#endif
            CHECK_RET_TAG_WITH_GOTO(avf_atempo, err);

            if (avfc)
                avf_link_tail(avfc, avf_atempo);
            else
                avfc = avf_atempo;
        }

        /* FIXME: soft vol need before eq/aef */
        if (o->vol_en) {
            avf_vol = avf_vol_open("vol", o->vol_index);
            CHECK_RET_TAG_WITH_GOTO(avf_vol, err);

            if (avfc)
                avf_link_tail(avfc, avf_vol);
            else
                avfc = avf_vol;
        }

        if (s16sf != osf) {
            swrp.osf = osf;
            swrp.isf = s16sf;
            avf_swr2 = avf_swr_open("swr2", &swrp);
            CHECK_RET_TAG_WITH_GOTO(avf_swr2, err);
            if (avfc)
                avf_link_tail(avfc, avf_swr2);
            else
                avfc = avf_swr2;
        }
    } else {
        if (o->ori_sf != osf) {
            /* swr only */
            swrp.osf = osf;
            swrp.isf = o->ori_sf;
            avf_swr1 = avf_swr_open("swr", &swrp);
            CHECK_RET_TAG_WITH_GOTO(avf_swr1, err);
            avfc = avfc ? avfc : avf_swr1;
        }
    }

    o->avfc       = avfc;
    o->avf_vol    = avf_vol;
    o->avf_atempo = avf_atempo;

    return 0;
err:
    avf_close(avf_atempo);
    avf_close(avf_vol);
    avf_close(avf_swr1);
    avf_close(avf_swr2);
    return -1;
}

static int _ao_filters_open2(ao_cls_t *o)
{
    swr_avfp_t swrp;
    sf_t s16sf, osf      = o->sf;
    avfilter_t *avfc     = NULL;
    avfilter_t *avf_swr1 = NULL, *avf_swr2 = NULL;
    avfilter_t *avf_eq   = NULL, *avf_aef = NULL, *avf_vol = NULL, *avf_atempo = NULL;

#ifdef AO_DEBUG
    if (!g_dump_data)
        g_dump_data = av_malloc(AO_DUMP_SIZE);
    g_dump_size = 0;
#endif

    if (o->eq_params || o->vol_en || o->aef_conf || o->atempo_play_en || CONFIG_AV_AEF_DEBUG) {
        s16sf = sf_make_channel(1) | sf_make_rate(sf_get_rate(o->ori_sf)) | sf_make_bit(16) | sf_make_signed(1);
        if (s16sf != o->ori_sf) {
            /* swr first */
            swrp.osf = s16sf;
            swrp.isf = o->ori_sf;
            avf_swr1 = avf_swr_open("swr1", &swrp);
            CHECK_RET_TAG_WITH_GOTO(avf_swr1, err);
            avfc = avfc ? avfc : avf_swr1;
        }

        /* FIXME: soft vol need before eq/aef */
        if (o->vol_en) {
            avf_vol = avf_vol_open("vol", o->vol_index);
            CHECK_RET_TAG_WITH_GOTO(avf_vol, err);

            if (avfc)
                avf_link_tail(avfc, avf_vol);
            else
                avfc = avf_vol;
        }

        if (o->atempo_play_en) {
            atempo_avfp_t atempop;

            memset(&atempop, 0, sizeof(atempo_avfp_t));
            atempop.sf    = s16sf;
            atempop.speed = o->speed;
#if ATEMPO_FREQ_REDUCE
            avf_atempo = avf_atempo2_open("atempo2", &atempop);
#else
            avf_atempo = avf_atempo_open("atempo", &atempop);
#endif
            CHECK_RET_TAG_WITH_GOTO(avf_atempo, err);

            if (avfc)
                avf_link_tail(avfc, avf_atempo);
            else
                avfc = avf_atempo;
        }

        if (o->eq_params) {
            avf_eq = _ao_eqx_new(o, s16sf);
            CHECK_RET_TAG_WITH_GOTO(avf_eq, err);
            if (avfc)
                avf_link_tail(avfc, avf_eq);
            else
                avfc = avf_eq;
        }

        if (o->aef_conf || CONFIG_AV_AEF_DEBUG) {
            aef_avfp_t aefp;
            memset(&aefp, 0, sizeof(aef_avfp_t));

            aefp.sf           = s16sf;
            aefp.conf         = o->aef_conf;
            aefp.conf_size    = o->aef_conf_size;
            aefp.nsamples_max = POST_PROC_SAMPLES_MAX;
            avf_aef = avf_aef_open("aef", &aefp);
            CHECK_RET_TAG_WITH_GOTO(avf_aef, err);
            if (avfc)
                avf_link_tail(avfc, avf_aef);
            else
                avfc = avf_aef;
        }

        if (s16sf != osf) {
            swrp.osf = osf;
            swrp.isf = s16sf;
            avf_swr2 = avf_swr_open("swr2", &swrp);
            CHECK_RET_TAG_WITH_GOTO(avf_swr2, err);
            if (avfc)
                avf_link_tail(avfc, avf_swr2);
            else
                avfc = avf_swr2;
        }
    } else {
        if (o->ori_sf != osf) {
            /* swr only */
            swrp.osf = osf;
            swrp.isf = o->ori_sf;
            avf_swr1 = avf_swr_open("swr", &swrp);
            CHECK_RET_TAG_WITH_GOTO(avf_swr1, err);
            avfc = avfc ? avfc : avf_swr1;
        }
    }

    o->avfc       = avfc;
    o->avf_vol    = avf_vol;
    o->avf_aef    = avf_aef;
    o->avf_atempo = avf_atempo;
    o->avf_eq     = o->eq_params ? avf_eq : NULL;

    return 0;
err:
    avf_close(avf_eq);
    avf_close(avf_aef);
    avf_close(avf_atempo);
    avf_close(avf_vol);
    avf_close(avf_swr1);
    avf_close(avf_swr2);
    return -1;
}

/**
 * @brief  open a audio out
 * @param  [in] sf     : audio sample format
 * @param  [in] ao_cnf
 * @return NULL on error
 */
ao_cls_t* ao_open(sf_t sf, const ao_conf_t *ao_cnf)
{
    int rc;
    sf_t osf;
    ao_cls_t *o      = NULL;
    mixer_cnl_t *cnl = NULL;
    avframe_t *frame = NULL;
    ao_conf_t ocnf, *iao_cnf = &ocnf;

    CHECK_PARAM(sf && ao_cnf && ao_cnf->name, NULL);
    CHECK_PARAM(ao_cnf->db_min != ao_cnf->db_max, NULL);
    _ao_init();
    vol_scale_init(ao_cnf->db_min, ao_cnf->db_max);
    o = av_zalloc(sizeof(ao_cls_t));
    CHECK_RET_TAG_WITH_RET(o, NULL);

    //patch for tmall resampler
    memcpy(iao_cnf, ao_cnf, sizeof(ao_conf_t));
    if (iao_cnf->eq_segments && iao_cnf->aef_conf && iao_cnf->aef_conf_size) {
        LOGE(TAG, "param faild, eq & aef can't both enabled");
        goto err;
    }
#if !(CONFIG_AO_MIXER_SUPPORT)
    if (iao_cnf->eq_segments) {
        o->eq_params = av_zalloc(sizeof(eqfp_t) * iao_cnf->eq_segments);
        CHECK_RET_TAG_WITH_GOTO(o->eq_params, err);
    }
    if (iao_cnf->aef_conf && iao_cnf->aef_conf_size) {
        o->aef_conf = av_malloc(iao_cnf->aef_conf_size);
        CHECK_RET_TAG_WITH_GOTO(o->aef_conf, err);
        memcpy(o->aef_conf, iao_cnf->aef_conf, iao_cnf->aef_conf_size);
        o->aef_conf_size = iao_cnf->aef_conf_size;
    }
#endif

    if (iao_cnf->resample_rate) {
        osf = sf_make_channel(CONFIG_AV_AO_CHANNEL_NUM) | sf_make_rate(iao_cnf->resample_rate) | sf_make_bit(16) | sf_make_signed(1);
    } else {
        osf = sf_make_channel(CONFIG_AV_AO_CHANNEL_NUM) | sf_make_rate(sf_get_rate(sf)) | sf_make_bit(16) | sf_make_signed(1);
    }

    frame = avframe_alloc();
    CHECK_RET_TAG_WITH_GOTO(frame, err);

    o->ops = _get_ao_ops_by_name(iao_cnf->name);
    if (!o->ops) {
        LOGE(TAG, "ao ops get failed, name = %s\n", iao_cnf->name);
        goto err;
    }
    o->oframe         = frame;
    o->ori_sf         = sf;
    o->resample_rate  = iao_cnf->resample_rate;
    o->eq_segments    = iao_cnf->eq_segments;
    o->period_ms      = iao_cnf->period_ms ? iao_cnf->period_ms : AO_ONE_PERIOD_MS;
    o->period_num     = iao_cnf->period_num ? iao_cnf->period_num : AO_TOTAL_PERIOD_NUM;
#if TMALL_PATCH
    o->vol_en         = 1;
#else
    o->vol_en         = iao_cnf->vol_en;
#endif
    o->vol_index      = iao_cnf->vol_index;
    o->atempo_play_en = iao_cnf->atempo_play_en;
    o->speed          = iao_cnf->speed;
#if CONFIG_AO_MIXER_SUPPORT
    o->sf = sf_make_channel(1) | sf_make_rate(sf_get_rate(osf)) | sf_make_bit(16) | sf_make_signed(1);

    ao_ref_lock();
    if (!ao_get_mixer()) {
        aos_task_t task;

        ao_set_mixer(mixer_new(o->sf));
        rc = aos_task_new_ext(&task, "mixer_ao", _mixer_ao_task, NULL, 3*1024, AOS_DEFAULT_APP_PRI - 2);
        if (rc != 0) {
            LOGE(TAG, "mixer_ao_task create faild, may be oom, rc = %d", rc);
        }
    } else {
        if (mixer_get_nb_cnls(ao_get_mixer()) > 0) {
            sf_t msf = mixer_get_sf(ao_get_mixer());

            if (sf_get_rate(msf) != sf_get_rate(o->sf)) {
                o->sf = sf_make_channel(1) | sf_make_rate(sf_get_rate(msf)) | sf_make_bit(16) | sf_make_signed(1);
                osf   = sf_make_channel(CONFIG_AV_AO_CHANNEL_NUM) | sf_make_rate(sf_get_rate(msf)) | sf_make_bit(16) | sf_make_signed(1);
                LOGW(TAG, "depend on the sf-mixer sf => %s", sf_get_format_str(msf));
                iao_cnf->resample_rate = sf_get_rate(msf);
            }
            rc = 0;
        } else {
            /* reconfig the sf */
            rc = mixer_set_sf(ao_get_mixer(), o->sf);
        }
    }

    if (rc == 0) {
        cnl = mixer_cnl_new(o->sf, MIXER_FIFO_SIZE_DEFAULT);
        if (cnl) {
            o->cnl = cnl;
            rc = mixer_attach(ao_get_mixer(), cnl);
        } else {
            rc = -1;
        }
    }
    ao_ref_unlock();
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = _ao_filters_open1(o);
#else
    o->sf = osf;
    rc = _ao_filters_open2(o);
#endif
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = __ao_open(o, o->sf, iao_cnf);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    LOGD(TAG, "ori sf ==> %s", sf_get_format_str(o->ori_sf));
    LOGD(TAG, "ao  sf ==> %s", sf_get_format_str(osf));
    aos_mutex_new(&o->lock);

    return o;
err:
    if (o) {
        av_free(o->eq_params);
        av_free(o->aef_conf);
        av_free(o);
    }
    if (cnl) {
#if CONFIG_AO_MIXER_SUPPORT
        ao_ref_lock();
        mixer_dettach(ao_get_mixer(), o->cnl);
        ao_ref_unlock();
        mixer_cnl_free(cnl);
#endif
    }
    avframe_free(&frame);
    return NULL;
}

/**
 * @brief  control the ao
 * @param  [in] o
 * @param  [in] cmd : command, see AO_CMD_XX
 * @param  [in] arg
 * @param  [in/out] arg_size
 * @return
 */
int ao_control(ao_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //FIXME:
    int rc = -1;
    size_t size;

    CHECK_PARAM(o, -1);
    o->interrupt = 1;
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    switch (cmd) {
#if CONFIG_AO_MIXER_SUPPORT
    case AO_CMD_EQ_ENABLE:
    case AO_CMD_EQ_SET_PARAM:
        rc = __ao_control(cmd, arg, arg_size);
        break;
#else
    case AO_CMD_EQ_ENABLE: {
        oeq_seten_t *val = arg;
        CHECK_RET_TAG_WITH_RET(val, -1);
        if (o->avf_eq) {
            eq_seten_t para;

            size = sizeof(para);
            para.enable = val->enable;
            rc = avf_control(o->avf_eq, AVF_CMD_EQ_SET_ENABLE, (void*)&para, &size);
        } else {
            rc = 0;
        }
        o->eq_en = (rc == 0) ? val->enable : o->eq_en;
    }
    break;
    case AO_CMD_EQ_SET_PARAM: {
        oeq_setpa_t *val = arg;
        CHECK_RET_TAG_WITH_RET(val, -1);
        if (!((val->segid < o->eq_segments) && o->eq_params)) {
            LOGE(TAG, "param faild. segid = %u, eq_segments = %u", val->segid, o->eq_segments);
            break;
        }

        if (o->avf_eq) {
            eq_setpa_t para;
            size       = sizeof(para);
            para.segid = val->segid;
            memcpy(&para.param, &val->param, sizeof(eqfp_t));
            rc = avf_control(o->avf_eq, AVF_CMD_EQ_SET_PARAM, (void*)&para, &size);
        } else {
            rc = 0;
        }
        if (rc == 0) {
            memcpy(&o->eq_params[val->segid], &val->param, sizeof(eqfp_t));
        }
    }
    break;
#endif
    case AO_CMD_VOL_SET: {
        ovol_set_t *val = arg;
        CHECK_RET_TAG_WITH_RET(val, -1);
        if (o->vol_en) {
            vol_set_t para;

            size = sizeof(para);
            para.vol_index = val->vol_index;
            rc = avf_control(o->avf_vol, AVF_CMD_VOL_SET, (void*)&para, &size);
            o->vol_index = (rc == 0) ? val->vol_index : o->vol_index;
        } else {
            rc = 0;
        }
    }
    break;
    case AO_CMD_ATEMPO_SET_SPEED: {
        if (arg) {
            float speed = *((float*)arg);
            if (o->atempo_play_en) {
                size = sizeof(speed);
                rc = avf_control(o->avf_atempo, AVF_CMD_ATEMPO_SET_SPEED, arg, arg_size);
                if (rc == 0) {
                    o->speed = speed;
#if XIAOYA_PATCH
                    {
                        int bypass = speed == 1 ? 0 : 1;
#if CONFIG_AO_MIXER_SUPPORT
                        ao_cls_t *ro;

                        ao_ref_lock();
                        ro = ao_get_real();

                        if (mixer_get_nb_cnls(ao_get_mixer()) > 1) {
                            avf_set_bypass(ao_get_real()->avf_aef, 1);
                        } else {
                            avf_set_bypass(ro->avf_aef, bypass);
                        }

                        ao_ref_unlock();
#else
                        avf_set_bypass(o->avf_aef, bypass);
#endif
                    }
#endif
                }
            } else {
                rc = 0;
            }
        }
    }
    break;
    default:
        break;
    }
    aos_mutex_unlock(&o->lock);
    o->interrupt = 0;

    return rc;
}

/**
 * @brief  start audio out
 * @param  [in] o
 * @return 0/-1
 */
int ao_start(ao_cls_t *o)
{
    int rc = 0;

    CHECK_PARAM(o, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    if (!o->start) {
        rc = __ao_start(o);
        if (rc < 0) {
            LOGE(TAG, "ao start failed, rc = %d", rc);
        } else {
            o->start = 1;
        }
    }
    aos_mutex_unlock(&o->lock);

    return rc;
}

/**
 * @brief  stop audio out
 * @param  [in] o
 * @return 0/-1
 */
int ao_stop(ao_cls_t *o)
{
    int rc = 0;

    CHECK_PARAM(o, -1);
    o->interrupt = 1;
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    if (o->start) {
        rc = __ao_stop(o);
        if (rc < 0) {
            LOGE(TAG, "ao stop failed, rc = %d", rc);
        } else {
            o->start = 0;
        }
    }
    aos_mutex_unlock(&o->lock);
    o->interrupt = 0;

    return rc;
}

static int _pcm_filter(ao_cls_t *o, const uint8_t *pcm, size_t pcm_size, uint8_t **dst, size_t *size)
{
    int rc;

    if (o->avfc) {
        avframe_t frame;

        *dst  = NULL;
        *size = 0;
        memset(&frame, 0, sizeof(avframe_t));
        frame.type        = AVMEDIA_TYPE_AUDIO;
        frame.data[0]     = (uint8_t*)pcm;
        frame.linesize[0] = pcm_size;
        frame.sf          = o->ori_sf;
        frame.nb_samples  = pcm_size / sf_get_frame_size(o->ori_sf);
        avframe_clear(o->oframe);
        rc = avf_filter_frame(o->avfc, (const avframe_t*)&frame, o->oframe);
        CHECK_RET_TAG_WITH_RET(rc >= 0, -1);
        if (o->oframe->sf && o->oframe->nb_samples) {
            *dst  = o->oframe->data[0];
            *size = o->oframe->linesize[0];
        }
    } else {
        *dst  = (uint8_t*)pcm;
        *size = pcm_size;
    }

    return 0;
}

/**
 * @brief  write data to audio out
 * @param  [in] o
 * @param  [in] buf
 * @param  [in] count
 * @return -1 when err
 */
int ao_write(ao_cls_t *o, const uint8_t *buf, size_t count)
{
    int rc = -1;
    uint8_t *pcm;
    size_t pcm_size;

    if (!(o && buf && count)) {
        LOGE(TAG, "param err, %s\n", __FUNCTION__);
        return -1;
    }
    //FIXME:
    if (o->interrupt) {
        LOGD(TAG, "write interrupt");
        return 0;
    }

    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    if (o->start) {
#ifdef AO_DEBUG
        if ((g_dump_size + count < AO_DUMP_SIZE) && g_dump_data) {
            memcpy(g_dump_data+g_dump_size, buf, count);
            g_dump_size += count;
        }
#endif
        rc = _pcm_filter(o, buf, count, &pcm, &pcm_size);
        if (rc < 0) {
            LOGE(TAG, "pcm convert fail, rc = %d", rc);
            goto quit;
        }
        if (!(pcm && pcm_size)) {
            //FIXME: inner filter need more data
            rc = count;
            goto quit;
        }
#if CONFIG_AO_MIXER_SUPPORT
        rc = mixer_cnl_write(o->cnl, pcm, pcm_size, 1000);
#else
        rc = __ao_write(o, pcm, pcm_size);
#endif
        rc = (rc == pcm_size) ? count : rc;
    } else {
        rc = 0;
        LOGI(TAG, "ao is not start yet!");
    }
quit:
    aos_mutex_unlock(&o->lock);

    return rc;
}

/**
 * @brief  drain out the tail pcm
 * @param  [in] o
 * @return 0/-1
 */
int ao_drain(ao_cls_t *o)
{
    int ret = -1;

    CHECK_PARAM(o, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    if (o->start) {
        ret = __ao_drain(o);
    } else {
        ret = 0;
        LOGI(TAG, "ao is not start yet!");
    }
    aos_mutex_unlock(&o->lock);

    return ret;
}

/**
 * @brief  close/destroy a audio out
 * @param  [in] o
 * @return 0/-1
 */
int ao_close(ao_cls_t *o)
{
    int ret = -1;

    CHECK_PARAM(o, -1);
#if CONFIG_AO_MIXER_SUPPORT
    ao_ref_lock();
    mixer_dettach(ao_get_mixer(), o->cnl);
#if XIAOYA_PATCH
    if (mixer_get_nb_cnls(ao_get_mixer()) == 1) {
        avf_set_bypass(ao_get_real()->avf_aef, 0);
    }
#endif
    ao_ref_unlock();
#endif
    ao_stop(o);
    __ao_close(o);

    avf_chain_close(o->avfc);
    avframe_free(&o->oframe);
    mixer_cnl_free(o->cnl);
    aos_mutex_free(&o->lock);
    av_free(o->eq_params);
    av_free(o->aef_conf);
    av_free(o);

    return ret;
}



