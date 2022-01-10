/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/av_config.h"
#include "avutil/common.h"
#include "avutil/av_typedef.h"
#include "stream/stream.h"
#include "avformat/avformat.h"
#include "avformat/avformat_utils.h"
#include "avformat/avparser_all.h"
#include "avcodec/avcodec.h"
#include "output/output.h"
#include "avfilter/avfilter.h"
#include "avfilter/avfilter_all.h"
#include "player.h"
#include <stdarg.h>

#define TAG                    "player"

enum player_status {
    PLAYER_STATUS_STOPED,
    PLAYER_STATUS_PREPARING,
    PLAYER_STATUS_PLAYING,
    PLAYER_STATUS_PAUSED,
};

#define player_lock()   (aos_mutex_lock(&player->lock, AOS_WAIT_FOREVER))
#define player_unlock() (aos_mutex_unlock(&player->lock))

#define EVENT_CALL(player, type, data, len) \
	do { \
		if (player && player->event_cb) \
			player->event_cb(player->user_data, type, data, len); \
	} while(0)

#ifndef __linux__
#define PLAYER_TASK_QUIT_EVT           (0x01)
#endif

struct player_cb {
    char                         *url;
    stream_cls_t                 *s;
    demux_cls_t                  *demuxer;
    ad_cls_t                     *ad;
    ao_cls_t                     *ao;
    char                         *ao_name;      ///< ao name

    uint64_t                     start_time;    ///< begin play time
    uint32_t                     cache_size;    ///< size of the web cache. 0 use default
    uint32_t                     cache_start_threshold; ///< (0~100)start read for player when up to cache_start_threshold. 0 use default
    uint32_t                     period_ms;     ///< period cache size(ms) for audio out. 0 means use default
    uint32_t                     period_num;    ///< number of period_ms. total cache size for ao is (period_num * period_ms * (rate / 1000) * 2 * (16/8)). 0 means use default
    uint32_t                     resample_rate; ///< none zereo means need to resample
    uint8_t                      vol_en;        ///< soft vol scale enable
    uint8_t                      vol_index;     ///< soft vol scale index (0~255)
    uint8_t                      atempo_play_en;///< atempo play enable
    float                        speed;         ///< atempo play speed.suggest: 0.5 ~ 2.0;
    uint8_t                      *aef_conf;     ///< config data for aef
    size_t                       aef_conf_size; ///< size of the config data for aef
    uint8_t                      eq_en;         ///< used for equalizer config
    uint8_t                      eq_segments;   ///< used for equalizer config
    eqfp_t                       *eq_params;
    get_decrypt_cb_t             get_dec_cb;    ///< used for get decrypt info

    int32_t                      db_min;        ///< min db for digital volume curve
    int32_t                      db_max;        ///< max db for digital volume curve

    int64_t                      cur_pts;
    uint8_t                      status;
    uint8_t                      before_status;

    aos_task_t                   ptask;
#ifndef __linux__
    aos_event_t                  evt;
#endif

    uint8_t                      evt_status;
    player_event_t               event_cb;
    void                         *user_data;    ///< user data of the player event
    uint8_t                      interrupt;
    uint8_t                      need_quit;

    aos_mutex_t                  lock;
    uint32_t                     rcv_timeout;

    struct {
        uint32_t  ao_write_size;
        uint32_t  run_loop;
        uint32_t  run_loop_valid;
        uint32_t  ao_full;
    }                            stat;
};

/**
 * @brief  init the player before once play
 * @param  [in] player
 * @return
 */
static void _player_inner_init(player_t *player)
{
    player->url           = NULL;
    player->s             = NULL;
    player->demuxer       = NULL;
    player->ad            = NULL;
    player->ao            = NULL;
    player->interrupt     = 0;
    player->need_quit     = 0;
    player->cur_pts       = 0;
    player->start_time    = 0;
    player->status        = PLAYER_STATUS_STOPED;
    player->before_status = PLAYER_STATUS_STOPED;
    player->evt_status    = PLAYER_EVENT_UNKNOWN;

#ifdef __linux__
    player->ptask         = 0;
#else
    aos_event_set(&player->evt, ~PLAYER_TASK_QUIT_EVT, AOS_EVENT_AND);
#endif
    memset(&player->stat, 0, sizeof(player->stat));
}

/**
 * @brief  init player module default
 * attention: can rewrite this function by caller
 * @return 0/-1
 */
__attribute__((weak)) int player_init()
{
    static int inited = 0;

    if (!inited) {
        resample_register();
        eqx_register();
        aefx_register();
        atempo_register();
        avparser_register_all();

        stream_register_all();
        demux_register_all();
        ad_register_all();
        ao_register_all();

        inited = 1;
    }

    return inited ? 0 : -1;
}

/**
 * @brief  init the player config param
 * @param  [in] ply_cnf
 * @return 0/-1
 */
int player_conf_init(ply_conf_t *ply_cnf)
{
    CHECK_PARAM(ply_cnf, -1);
    memset(ply_cnf, 0, sizeof(ply_conf_t));
    ply_cnf->ao_name               = "alsa";
    ply_cnf->speed                 = 1;
    ply_cnf->rcv_timeout           = CONFIG_AV_STREAM_RCV_TIMEOUT_DEFAULT;
    ply_cnf->cache_size            = CONFIG_AV_STREAM_CACHE_SIZE_DEFAULT;
    ply_cnf->cache_start_threshold = CONFIG_AV_STREAM_CACHE_THRESHOLD_DEFAULT;
    ply_cnf->period_ms             = AO_ONE_PERIOD_MS;
    ply_cnf->period_num            = AO_TOTAL_PERIOD_NUM;
    ply_cnf->db_min                = VOL_SCALE_DB_MIN;
    ply_cnf->db_max                = VOL_SCALE_DB_MAX;

    return 0;
}

/**
 * @brief  new a player obj
 * @param  [in] ply_cnf
 * @return NULL on error
 */
player_t* player_new(const ply_conf_t *ply_cnf)
{
    player_t *player = NULL;

    CHECK_PARAM(ply_cnf && ply_cnf->ao_name && ply_cnf->speed >= PLAY_SPEED_MIN && ply_cnf->speed <= PLAY_SPEED_MAX, NULL);
    CHECK_PARAM(ply_cnf->db_min != ply_cnf->db_max, NULL);
    LOGI(TAG, "%s, %d enter.", __FUNCTION__, __LINE__);
    player = (struct player_cb*)av_zalloc(sizeof(struct player_cb));
    CHECK_RET_TAG_WITH_RET(player, NULL);
    player->ao_name = strdup(ply_cnf->ao_name);
    CHECK_RET_TAG_WITH_GOTO(player->ao_name, err);

#if 1
    if (ply_cnf->eq_segments && ply_cnf->aef_conf && ply_cnf->aef_conf_size) {
        LOGE(TAG, "param faild, eq & aef can't both enabled");
        goto err;
    }
#endif
    if (ply_cnf->eq_segments) {
        player->eq_params = av_zalloc(sizeof(eqfp_t) * ply_cnf->eq_segments);
        CHECK_RET_TAG_WITH_GOTO(player->eq_params, err);
    }
    if (ply_cnf->aef_conf && ply_cnf->aef_conf_size) {
        player->aef_conf = av_malloc(ply_cnf->aef_conf_size);
        CHECK_RET_TAG_WITH_GOTO(player->aef_conf, err);
        memcpy(player->aef_conf, ply_cnf->aef_conf, ply_cnf->aef_conf_size);
        player->aef_conf_size = ply_cnf->aef_conf_size;
    }

    player->user_data             = ply_cnf->user_data;
    player->event_cb              = ply_cnf->event_cb;
    player->get_dec_cb            = ply_cnf->get_dec_cb;
    player->resample_rate         = ply_cnf->resample_rate;
    player->vol_en                = ply_cnf->vol_en;
    player->vol_index             = ply_cnf->vol_index;
    player->atempo_play_en        = ply_cnf->atempo_play_en;
    player->speed                 = ply_cnf->speed;
    player->rcv_timeout           = ply_cnf->rcv_timeout ? ply_cnf->rcv_timeout : CONFIG_AV_STREAM_RCV_TIMEOUT_DEFAULT;
    player->eq_segments           = ply_cnf->eq_segments;
    player->cache_size            = ply_cnf->cache_size ? ply_cnf->cache_size : CONFIG_AV_STREAM_CACHE_SIZE_DEFAULT;
    player->cache_start_threshold = ply_cnf->cache_start_threshold ? ply_cnf->cache_start_threshold : CONFIG_AV_STREAM_CACHE_THRESHOLD_DEFAULT;
    player->period_ms             = ply_cnf->period_ms ? ply_cnf->period_ms : AO_ONE_PERIOD_MS;
    player->period_num            = ply_cnf->period_num ? ply_cnf->period_num : AO_TOTAL_PERIOD_NUM;
    player->db_min                = ply_cnf->db_min;
    player->db_max                = ply_cnf->db_max;
#ifndef __linux__
    aos_event_new(&player->evt, 0);
#endif
    aos_mutex_new(&player->lock);
    _player_inner_init(player);

    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    return player;
err:
    if (player) {
        av_free(player->ao_name);
        av_free(player->eq_params);
        av_free(player->aef_conf);
        av_free(player);
    }
    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    return NULL;
}

/**
 * @brief  control/config the player by command
 * @PARAM  [in] player
 * @PARAM  [in] cmd    : PLAYER_CMD_XXX
 * @param  [in] ...
 * @return
 */
int player_ioctl(player_t *player, int cmd, ...)
{
    int rc = -1;
    size_t size;
    va_list ap;

    CHECK_PARAM(player, -1);
    player_lock();
    player->interrupt = 1;
    LOGD(TAG, "%s, %d enter. player = %p, cmd = %d", __FUNCTION__, __LINE__, player, cmd);
    switch (cmd) {
    case PLAYER_CMD_SET_RESAMPLE_RATE:
    case PLAYER_CMD_SET_RCVTO:
    case PLAYER_CMD_SET_CACHE_SIZE: {
        uint32_t val;
        va_start(ap, cmd);
        val = va_arg(ap, uint32_t);
        va_end(ap);
        if (cmd == PLAYER_CMD_SET_RESAMPLE_RATE)
            player->resample_rate = val;
        else if (cmd == PLAYER_CMD_SET_RCVTO)
            player->rcv_timeout = val ? val : CONFIG_AV_STREAM_RCV_TIMEOUT_DEFAULT;
        else
            player->cache_size = val ? val : CONFIG_AV_STREAM_CACHE_SIZE_DEFAULT;
        rc = 0;
    }
    break;
    case PLAYER_CMD_EQ_ENABLE: {
        peq_seten_t *val;
        va_start(ap, cmd);
        val = va_arg(ap, peq_seten_t*);
        va_end(ap);
        CHECK_RET_TAG_WITH_RET(val, -1);
        if (player->ao) {
            oeq_seten_t para;

            size = sizeof(para);
            para.enable = val->enable;
            rc = ao_control(player->ao, AO_CMD_EQ_ENABLE, (void*)&para, &size);
        } else {
            rc = 0;
        }
        player->eq_en = (rc == 0) ? val->enable : player->eq_en;
    }
    break;
    case PLAYER_CMD_EQ_SET_PARAM: {
        eqfp_t *param;
        peq_setpa_t *val;
        va_start(ap, cmd);
        val = va_arg(ap, peq_setpa_t*);
        va_end(ap);
        CHECK_RET_TAG_WITH_RET(val, -1);

        if (!((val->segid < player->eq_segments) && player->eq_params)) {
            LOGE(TAG, "param faild. segid = %u, eq_segments = %u", val->segid, player->eq_segments);
            break;
        }
        param = &val->param;
        if (!(param->gain >= -24.0 && param->gain <= 18.0
              && param->q >= 0.1 && param->q <= 50.0
              && param->type > EQF_TYPE_UNKNOWN && param->type < EQF_TYPE_MAX)) {
            LOGE(TAG, "eq param error");
            break;
        }

        if (player->ao) {
            oeq_setpa_t para;
            size       = sizeof(para);
            para.segid = val->segid;
            memcpy(&para.param, &val->param, sizeof(eqfp_t));
            rc = ao_control(player->ao, AO_CMD_EQ_SET_PARAM, (void*)&para, &size);
        } else {
            rc = 0;
        }
        if (rc == 0) {
            memcpy(&player->eq_params[val->segid], &val->param, sizeof(eqfp_t));
        }
    }
    break;
    default:
        break;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player->interrupt = 0;
    player_unlock();

    return rc;
}

static int _interrupt(void *arg)
{
    player_t *player = arg;

    return player->need_quit;
}

static ao_cls_t* _player_ao_new(player_t *player, sf_t ao_sf)
{
    int i, rc;
    ao_conf_t ao_cnf;
    size_t size;
    ao_cls_t *ao;

    ao_conf_init(&ao_cnf);
    ao_cnf.name           = player->ao_name;
    ao_cnf.eq_segments    = player->eq_segments;
    ao_cnf.resample_rate  = player->resample_rate;
    ao_cnf.aef_conf       = player->aef_conf;
    ao_cnf.aef_conf_size  = player->aef_conf_size;
    ao_cnf.vol_en         = player->vol_en;
    ao_cnf.vol_index      = player->vol_index;
    ao_cnf.atempo_play_en = player->atempo_play_en;
    ao_cnf.speed          = player->speed;
    ao_cnf.period_ms      = player->period_ms;
    ao_cnf.period_num     = player->period_num;
    ao_cnf.db_min         = player->db_min;
    ao_cnf.db_max         = player->db_max;
    ao = ao_open(ao_sf, &ao_cnf);
    CHECK_RET_TAG_WITH_RET(ao, NULL);

    if (player->eq_params) {
        oeq_seten_t eqen;
        oeq_setpa_t eqpa;

        size = sizeof(oeq_setpa_t);
        for (i = 0; i < player->eq_segments; i++) {
            if (player->eq_params[i].type != EQF_TYPE_UNKNOWN) {
                eqpa.segid = i;
                memcpy(&eqpa.param, (void*)&player->eq_params[i], sizeof(eqfp_t));
                rc = ao_control(ao, AO_CMD_EQ_SET_PARAM, (void*)&eqpa, &size);
                CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
            }
        }

        size        = sizeof(oeq_seten_t);
        eqen.enable = player->eq_en;
        rc          = ao_control(ao, AO_CMD_EQ_ENABLE, (void*)&eqen, &size);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    }

    return ao;
err:
    ao_close(ao);
    return NULL;
}

static void _stream_event(void *opaque, uint8_t event, const void *data, uint32_t len)
{
    player_t *player = opaque;

    if (player->status == PLAYER_STATUS_PLAYING) {
        switch (event) {
        case STREAM_EVENT_UNDER_RUN:
            player->evt_status = PLAYER_EVENT_UNDER_RUN;
            break;
        case STREAM_EVENT_OVER_RUN:
            player->evt_status = PLAYER_EVENT_OVER_RUN;
            break;
        default:
            LOGE(TAG, "stream event unknown, event = %u", event);
            return;
        }
        EVENT_CALL(player, player->evt_status, NULL, 0);
    }
}

static int _player_prepare(player_t *player)
{
    int rc;
    sf_t sf;
    ad_conf_t ad_cnf;
    stm_conf_t stm_cnf;
    stream_cls_t  *s       = NULL;
    demux_cls_t   *demuxer = NULL;
    ad_cls_t      *ad      = NULL;
    ao_cls_t      *ao      = NULL;

    stream_conf_init(&stm_cnf);
    stm_cnf.rcv_timeout           = player->rcv_timeout;
    stm_cnf.get_dec_cb            = player->get_dec_cb;
    stm_cnf.cache_size            = player->cache_size;
    stm_cnf.cache_start_threshold = player->cache_start_threshold;
    stm_cnf.irq.arg               = player;
    stm_cnf.irq.handler           = _interrupt;
    stm_cnf.opaque                = player;
    stm_cnf.stream_event_cb       = _stream_event;
    s = stream_open(player->url, &stm_cnf);
    CHECK_RET_TAG_WITH_GOTO(s, err);
    player->speed = stream_is_live(s) ? 1 : player->speed;
    demuxer = demux_open(s);
    CHECK_RET_TAG_WITH_GOTO(demuxer, err);
    if (player->start_time) {
        rc = demux_seek(demuxer, player->start_time);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    }

    ad_conf_init(&ad_cnf);
    ad_cnf.sf             = demuxer->ash.sf;
    ad_cnf.extradata      = demuxer->ash.extradata;
    ad_cnf.extradata_size = demuxer->ash.extradata_size;
    ad_cnf.block_align    = demuxer->ash.block_align;
    ad_cnf.bps            = demuxer->ash.bps;
    ad = ad_open(demuxer->ash.id, &ad_cnf);
    CHECK_RET_TAG_WITH_GOTO(ad, err);

    /* FIXME: sf of the demuxer may be inaccurate */
    sf = ad->ash.sf ? ad->ash.sf : demuxer->ash.sf;

    ao = _player_ao_new(player, sf);
    CHECK_RET_TAG_WITH_GOTO(ao, err);
    rc = ao_start(ao);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    player->s       = s;
    player->demuxer = demuxer;
    player->ad      = ad;
    player->ao      = ao;

    return 0;
err:
    ao_close(ao);
    ad_close(ad);
    demux_close(demuxer);
    stream_close(s);
    return -1;
}

static void _ptask(void *arg)
{
    avpacket_t pkt;
    uint8_t play_first = 1;
    avframe_t *dframe;
    ad_cls_t *ad;
    demux_cls_t *demuxer;
    player_t *player = arg;
    int rc = -1, got_frame = 0;

    avpacket_init(&pkt);
    dframe = avframe_alloc();
    CHECK_RET_TAG_WITH_GOTO(dframe, quit);

    /* FIXME: prepare may be block too long */
    rc = _player_prepare(player);
    if (rc < 0) {
        goto quit;
    }

loop:
    player_lock();
    if (player->status == PLAYER_STATUS_PREPARING) {
        player->status = PLAYER_STATUS_PLAYING;
        player_unlock();
    } else if (player->status == PLAYER_STATUS_PAUSED) {
        player_unlock();
        aos_msleep(200);
        goto loop;
    } else {
        /* player status is PLAYER_STATUS_STOPED only */
        rc = 0;
        player_unlock();
        goto quit;
    }

    ad      = player->ad;
    demuxer = player->demuxer;
    EVENT_CALL(player, PLAYER_EVENT_START, NULL, 0);

    for (;;) {
        player->stat.run_loop++;
        if (PLAYER_STATUS_PAUSED == player->status) {
            aos_msleep(200);
            continue;
        } else if (PLAYER_STATUS_STOPED == player->status) {
            break;
        }

        //FIXME: for scheduler reason
        if (player->interrupt) {
            aos_msleep(5);
            continue;
        }

        if (!pkt.len) {
            rc = demux_read_packet(demuxer, &pkt);
            if (rc < 0) {
                LOGE(TAG, "read packet fail, rc = %d", rc);
                goto quit;
            } else if (rc == 0) {
                break;
            }
            player->cur_pts = pkt.pts;
        }

        if (player->status == PLAYER_STATUS_PLAYING) {
            rc = ad_decode(ad, dframe, &got_frame, &pkt);
            if (rc <= 0) {
                LOGE(TAG, "ad decode fail, rc = %d", rc);
                AV_ERRNO_SET(AV_ERRNO_DECODE_FAILD);
                goto quit;
            }
            pkt.len = 0;
            if (!got_frame) {
                continue;
            }

            rc = ao_write(player->ao, dframe->data[0], dframe->linesize[0]);
            if (rc >= 0) {
                player->stat.ao_write_size += rc;
                player->stat.run_loop_valid++;
                if (play_first == 1) {
                    play_first = 0;
                    LOGI(TAG, "first frame output");
                }
            } else {
                LOGE(TAG, "ao write fail, rc = %d, pcm_size = %d", rc, dframe->linesize[0]);
                AV_ERRNO_SET(AV_ERRNO_OUTPUT_FAILD);
                goto quit;
            }
        }
    }
    rc = 0;
quit:
    LOGD(TAG, "cb run task quit");
    avpacket_free(&pkt);
    avframe_free(&dframe);
#ifndef __linux__
    aos_event_set(&player->evt, PLAYER_TASK_QUIT_EVT, AOS_EVENT_OR);
#endif
    if ((player->status != PLAYER_STATUS_STOPED) && (player->need_quit != 1)) {
        player->evt_status = (rc < 0) ? PLAYER_EVENT_ERROR : PLAYER_EVENT_FINISH;
        EVENT_CALL(player, player->evt_status, NULL, 0);
    }
    return;
}

/**
 * @brief  player play interface
 * @param  [in] player
 * @param  [in] url        : example: http://ip:port/xx.mp3
 * @param  [in] start_time : begin play time, ms
 * @return 0/-1
 */
int player_play(player_t *player, const char *url, uint64_t start_time)
{
    int rc = -1;

    if (!(player && url && strlen(url))) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }

    player_lock();
    LOGI(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    if (player->status != PLAYER_STATUS_STOPED) {
        LOGE(TAG, "the player: %p is not stopped!", player);
        goto quit;
    }
    _player_inner_init(player);
    player->start_time = start_time;
    player->url        = strdup(url);
    CHECK_RET_TAG_WITH_GOTO(player->url, quit);
    player->status = PLAYER_STATUS_PREPARING;
    rc = aos_task_new_ext(&player->ptask, "player_task", _ptask, (void *)player, CONFIG_PLAYER_TASK_STACK_SIZE, AOS_DEFAULT_APP_PRI - 2);
    if (rc != 0) {
        av_freep(&player->url);
        player->status = PLAYER_STATUS_STOPED;
        LOGE(TAG, "player_task new create faild, may be oom, rc = %d", rc);
        AV_ERRNO_SET(AV_ERRNO_OOM);
        goto quit;
    }

quit:
    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player_unlock();
    return rc;
}

/**
 * @brief  pause the player
 * @param  [in] player
 * @return 0/-1
 */
int player_pause(player_t *player)
{
    int ret = -1;

    CHECK_PARAM(player, -1);
    player_lock();
    player->interrupt = 1;
    LOGI(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    switch (player->status) {
    case PLAYER_STATUS_PLAYING:
        ao_stop(player->ao);
    case PLAYER_STATUS_PREPARING:
        player->before_status = player->status;
        player->status        = PLAYER_STATUS_PAUSED;
        ret = 0;
        break;
    default:
        break;
    }
    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player->interrupt = 0;
    player_unlock();

    return ret;
}

/**
 * @brief  resume the player
 * @param  [in] player
 * @return 0/-1
 */
int player_resume(player_t *player)
{
    int ret = -1;

    CHECK_PARAM(player, -1);
    player_lock();
    player->interrupt = 1;
    LOGI(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    if (player->status == PLAYER_STATUS_PAUSED) {
        switch (player->before_status) {
        case PLAYER_STATUS_PLAYING:
            ao_start(player->ao);
        case PLAYER_STATUS_PREPARING:
            player->status        = player->before_status;
            player->before_status = PLAYER_STATUS_PAUSED;
            ret = 0;
            break;
        default:
            break;
        }
    }
    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player->interrupt = 0;
    player_unlock();

    return ret;
}

static int _player_stop(player_t *player)
{
    int ret = -1;

    if (player->ao) {
        /* play finish normal */
        if (player->evt_status == PLAYER_EVENT_FINISH) {
            ao_drain(player->ao);
        }
        ao_stop(player->ao);
        ao_close(player->ao);
        player->ao = NULL;
    }
    if (player->ad) {
        ad_close(player->ad);
        player->ad = NULL;
    }
    if (player->demuxer) {
        demux_close(player->demuxer);
        player->demuxer = NULL;
    }
    if (player->s) {
        stream_close(player->s);
        player->s = NULL;
    }

    ret = 0;
    av_freep(&player->url);

    return ret;
}

/**
 * @brief  stop the player
 * @param  [in] player
 * @return 0/-1
 */
int player_stop(player_t *player)
{
    int ret = 0;

    CHECK_PARAM(player, -1);
    player->need_quit = 1;
    player_lock();
    LOGI(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    if (player->status != PLAYER_STATUS_STOPED) {
        player->status = PLAYER_STATUS_STOPED;
        player_unlock();
#ifdef __linux__
        if (player->ptask) {
            pthread_join(player->ptask, NULL);
            player->ptask = 0;
        }
#else
        {
            unsigned int flag;
            aos_event_get(&player->evt, PLAYER_TASK_QUIT_EVT, AOS_EVENT_OR_CLEAR, &flag, AOS_WAIT_FOREVER);
        }
#endif
        ret = _player_stop(player);
        _player_inner_init(player);
    } else {
        player_unlock();
    }

    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    return ret;
}

/**
 * @brief  free/destroy the player obj
 * @param  [in] player
 * @return 0/-1
 */
int player_free(player_t *player)
{
    int rc = 0;

    CHECK_PARAM(player, -1);
    LOGI(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    if (player->status != PLAYER_STATUS_STOPED) {
        rc = player_stop(player);
    }

#ifndef __linux__
    aos_event_free(&player->evt);
#endif
    aos_mutex_free(&player->lock);
    av_free(player->ao_name);
    av_free(player->aef_conf);
    av_free(player);

    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    return rc;
}

/**
 * @brief  seek to the time
 * @param  [in] player
 * @param  [in] timestamp : seek time
 * @return 0/-1
 */
int player_seek(player_t *player, uint64_t timestamp)
{
    int rc = -1;
    demux_cls_t *demuxer;

    CHECK_PARAM(player, -1);
    player_lock();
    player->interrupt = 1;
    LOGI(TAG, "%s, %d enter. player = %p, timestamp = %llu", __FUNCTION__, __LINE__, player, timestamp);
    demuxer = player->demuxer;
    if (player->status == PLAYER_STATUS_PLAYING
        || (player->status == PLAYER_STATUS_PAUSED && player->before_status != PLAYER_STATUS_PREPARING)) {
        ao_stop(player->ao);
        rc = demux_seek(demuxer, timestamp);
        if (rc == 0) {
            player->cur_pts = demuxer->time_scale ? timestamp * (demuxer->time_scale / 1000) : player->cur_pts;
            ad_reset(player->ad);
        }
        ao_start(player->ao);
    }
    LOGI(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player->interrupt = 0;
    player_unlock();

    return rc;
}

/**
 * @brief  get current play time
 * @param  [in] player
 * @param  [in/out] ptime
 * @return 0/-1
 */
int player_get_cur_ptime(player_t *player, play_time_t *ptime)
{
    int rc = -1;
    demux_cls_t *demuxer;

    CHECK_PARAM(player && ptime, -1);
    //player_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    demuxer = player->demuxer;
    if ((player->status == PLAYER_STATUS_PLAYING || (player->status == PLAYER_STATUS_PAUSED && player->before_status != PLAYER_STATUS_PREPARING))
        && demuxer) {
        ptime->curtime  = demuxer->time_scale ? player->cur_pts * 1000 / demuxer->time_scale : 0;
        ptime->duration = demuxer->duration;
        rc              = 0;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    //player_unlock();

    return rc;
}

/**
 * @brief  get media info
 * @param  [in] player
 * @param  [in/out] minfo : need free by the caller(use media_info_uninit function)
 * @return 0/-1
 */
int player_get_media_info(player_t *player, media_info_t *minfo)
{
    int rc = -1;
    demux_cls_t *demuxer;

    CHECK_PARAM(player && minfo, -1);
    //player_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    demuxer = player->demuxer;
    memset(minfo, 0, sizeof(media_info_t));
    if ((player->status == PLAYER_STATUS_PLAYING || (player->status == PLAYER_STATUS_PAUSED && player->before_status != PLAYER_STATUS_PREPARING))
        && demuxer && player->s) {
        minfo->tracks   = tracks_info_dup(demuxer->tracks);
        minfo->size     = stream_get_size(player->s);
        //TODO: just one track now for audio player
        minfo->bps      = demuxer->bps;
        minfo->duration = demuxer->duration;
        rc              = 0;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    //player_unlock();

    return rc;
}

/**
 * @brief  get soft vol index of the player
 * @param  [in] player
 * @param  [in/out] vol : vol scale index(0~255)
 * @return 0/-1
 */
int player_get_vol(player_t *player, uint8_t *vol)
{
    int rc = -1;

    CHECK_PARAM(player && vol, -1);
    player_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    if (player->vol_en) {
        *vol = player->vol_index;
        rc = 0;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player_unlock();

    return rc;
}

/**
 * @brief  set soft vol index of the player
 * @param  [in] player
 * @param  [in/out] vol : vol scale index(0~255)
 * @return 0/-1
 */
int player_set_vol(player_t *player, uint8_t vol)
{
    int rc = -1;

    CHECK_PARAM(player, -1);
    player_lock();
    player->interrupt = 1;
    //LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    if (player->vol_en) {
        if (player->ao) {
            size_t size;
            ovol_set_t para;

            size = sizeof(para);
            para.vol_index = vol;
            rc = ao_control(player->ao, AO_CMD_VOL_SET, (void*)&para, &size);
            player->vol_index = rc < 0 ? player->vol_index : vol;
        } else {
            player->vol_index = vol;
            rc = 0;
        }
    }
    //LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player->interrupt = 0;
    player_unlock();

    return rc;
}

/**
 * @brief  get play source url
 * @param  [in] player
 * @return NULL on error
 */
const char* player_get_url(player_t *player)
{
    CHECK_PARAM(player, NULL);
    return player->url;
}

/**
 * @brief  get play speed of the player
 * @param  [in] player
 * @param  [out] speed
 * @return 0/-1
 */
int player_get_speed(player_t *player, float *speed)
{
    CHECK_PARAM(player && speed, -1);
    player_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    *speed = player->speed;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player_unlock();

    return 0;
}

/**
 * @brief  set play speed of the player
 * @param  [in] player
 * @param  [in] speed : [PLAY_SPEED_MIN ~ PLAY_SPEED_MAX]
 * @return 0/-1
 */
int player_set_speed(player_t *player, float speed)
{
    int rc = -1;

    CHECK_PARAM(player && speed >= PLAY_SPEED_MIN && speed <= PLAY_SPEED_MAX, -1);
    player_lock();
    player->interrupt = 1;
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, player);
    if (player->atempo_play_en) {
        if (player->ao) {
            //live is not allowed set speed
            if (!stream_is_live(player->s)) {
                size_t size;

                size = sizeof(float);
                rc = ao_control(player->ao, AO_CMD_ATEMPO_SET_SPEED, (void*)&speed, &size);
                player->speed = rc < 0 ? player->speed : speed;
            }
        } else {
            player->speed = speed;
            rc = 0;
        }
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    player->interrupt = 0;
    player_unlock();

    return rc;
}


