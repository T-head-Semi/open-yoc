/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avfilter/avfilter_vol.h"
#include "avutil/vol_scale.h"

#define TAG                  "filter_vol"

struct avfilter_vol {
    struct avfilter           f;

    uint8_t                   vol_index;  ///< soft vol scale index (0~255)
};

#if TMALL_PATCH
static struct {
    uint8_t                   vol_index;  ///< soft vol scale index (0~255)
    aos_mutex_t               lock;
} g_tvol;

static int _tmall_vol_init()
{
    static int init;

    if (!init) {
        aos_mutex_new(&g_tvol.lock);
        g_tvol.vol_index = 160;
        init = 1;
    }

    return init ? 0 : -1;
}

/**
 * @brief  set soft vol for tmall
 * @param  [in] vol_index : 0~255
 * @return 0/-1
 */
int tmall_vol_set(uint8_t vol_index)
{
    _tmall_vol_init();
    aos_mutex_lock(&g_tvol.lock, AOS_WAIT_FOREVER);
    g_tvol.vol_index = vol_index;
    aos_mutex_unlock(&g_tvol.lock);
    return 0;
}

/**
 * @brief  get soft vol for tmall
 * @return 0~255
 */
uint8_t tmall_vol_get()
{
    uint8_t vol_index;

    _tmall_vol_init();
    aos_mutex_lock(&g_tvol.lock, AOS_WAIT_FOREVER);
    vol_index = g_tvol.vol_index;
    aos_mutex_unlock(&g_tvol.lock);

    return vol_index;
}

#endif

static int _avf_vol_init(avfilter_t *avf)
{
    //TODO
    return 0;
}

static int _avf_vol_control(avfilter_t *avf, int cmd, void *arg, size_t *arg_size)
{
    int rc = -1;
    struct avfilter_vol *dobj = (struct avfilter_vol*)avf;

    switch (cmd) {
    case AVF_CMD_VOL_SET: {
        vol_set_t *avfp = arg;
        dobj->vol_index = avfp->vol_index;
        rc              = 0;
    }
    break;
    default:
        break;
    }

    return rc;
}

static int _avf_vol_filter_frame(avfilter_t *avf, const avframe_t *in, avframe_t *out)
{
    int rc = -1;
    struct avfilter_vol *dobj = (struct avfilter_vol*)avf;

    if (get_esf_type(in->sf) == ESF_S16LE) {
        out->sf         = in->sf;
        out->nb_samples = in->nb_samples;
        rc = avframe_get_buffer(out);
        if (rc < 0) {
            LOGE(TAG, "may be oom, rc = %d", rc);
            return -1;
        }
#if TMALL_PATCH
        {
            dobj->vol_index = tmall_vol_get();
            rc = vol_scale((const int16_t*)in->data[0], in->nb_samples, (int16_t*)out->data[0], dobj->vol_index);
            CHECK_RET_TAG_WITH_RET(rc == 0, -1);
        }
#else
        rc = vol_scale((const int16_t*)in->data[0], in->nb_samples, (int16_t*)out->data[0], dobj->vol_index);
        CHECK_RET_TAG_WITH_RET(rc == 0, -1);
#endif

        rc = out->nb_samples;
    }

    return rc;
}

static int _avf_vol_uninit(avfilter_t *avf)
{
    //TODO
    return 0;
}

const struct avfilter_ops avf_ops_vol = {
    .name            = "vol",
    .type            = AVMEDIA_TYPE_AUDIO,

    .init            = _avf_vol_init,
    .control         = _avf_vol_control,
    .filter_frame    = _avf_vol_filter_frame,
    .uninit          = _avf_vol_uninit,
};

/**
 * @brief  open vol avfilter
 * @param  [in] inst_name : the name of avfilter_t instance
 * @param  [in] vol_index : soft vol scale index (0~255)
 * @return NULL on error
 */
avfilter_t *avf_vol_open(const char *inst_name, uint8_t vol_index)
{
    int rc;
    struct avfilter *f        = NULL;
    struct avfilter_vol *dobj = NULL;

    CHECK_PARAM(inst_name, NULL);
    dobj = av_zalloc(sizeof(struct avfilter_vol));
    CHECK_RET_TAG_WITH_RET(dobj, NULL);
    f       = (avfilter_t*)dobj;
    f->ops  = &avf_ops_vol;
    f->dobj = dobj;

    rc = avf_init(inst_name, f);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = f->ops->init ? f->ops->init((avfilter_t*)dobj) : 0;
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    dobj->vol_index = vol_index;
    LOGD(TAG, "open a avfilter, name = %s", f->ops->name);

    return (avfilter_t*)dobj;
err:
    avf_uninit(f);
    av_free(dobj);
    return NULL;
}


