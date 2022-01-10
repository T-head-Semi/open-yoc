/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avfilter/avfilter_atempo.h"
#include "atempo/atempo.h"

#define TAG                   "filter_atempo"

struct avfilter_atempo {
    struct avfilter           f;

    sf_t                      sf;
    float                     speed;          ///< suggest: 0.5 ~ 2.0;
    uint8_t                   *obuf;
    size_t                    osize;
    atempo_t                  *atempo;
};

static int _avf_atempo_init(avfilter_t *avf)
{
    //TODO
    return 0;
}

static int _avf_atempo_control(avfilter_t *avf, int cmd, void *arg, size_t *arg_size)
{
    int rc = -1;
    struct avfilter_atempo *dobj = (struct avfilter_atempo*)avf;
    atempo_t *atempo             = dobj->atempo;

    switch (cmd) {
    case AVF_CMD_ATEMPO_SET_SPEED: {
        if (arg) {
            float speed = *((float*)arg);

            rc = atempo_set_speed(atempo, speed);
            if (rc == 0) {
                dobj->speed = speed;
                avf->bypass = speed == 1 ? 1 : 0;
            }
        }
    }
    break;
    default:
        break;
    }

    return rc;
}

static int _avf_atempo_filter_frame(avfilter_t *avf, const avframe_t *in, avframe_t *out)
{
    int rc = -1, size;
    struct avfilter_atempo *dobj = (struct avfilter_atempo*)avf;
    atempo_t *atempo             = dobj->atempo;

    if (atempo && (in->sf == dobj->sf)) {
        //FIXME:
        size = (1024 + in->nb_samples / dobj->speed) * sizeof(int16_t);
        if (size > dobj->osize) {
            void *data = av_realloc(dobj->obuf, size);
            CHECK_RET_TAG_WITH_RET(data, -1);

            dobj->obuf  = data;
            dobj->osize = size;
        }

        atempo_write(atempo, (const int16_t*)in->data[0], in->nb_samples);
        //FIXME: may not read eof once.
        rc = atempo_read(atempo, (int16_t*)dobj->obuf, dobj->osize / sizeof(int16_t));
        if (rc <= 0) {
            //LOGD(TAG, "may be error, rc = %d", rc);
            /* return 0 may be need more pcm data */
            return rc;
        }

        out->sf         = dobj->sf;
        out->nb_samples = rc;
        rc = avframe_get_buffer(out);
        if (rc < 0) {
            LOGE(TAG, "may be oom, rc = %d", rc);
            return -1;
        }
        memcpy(out->data[0], dobj->obuf, out->nb_samples * sizeof(int16_t));
        rc = out->nb_samples;
    }

    return rc;
}

static int _avf_atempo_uninit(avfilter_t *avf)
{
    struct avfilter_atempo *dobj = (struct avfilter_atempo*)avf;

    atempo_free(dobj->atempo);
    av_free(dobj->obuf);
    return 0;
}

const struct avfilter_ops avf_ops_atempo = {
    .name            = "atempo",
    .type            = AVMEDIA_TYPE_AUDIO,

    .init            = _avf_atempo_init,
    .control         = _avf_atempo_control,
    .filter_frame    = _avf_atempo_filter_frame,
    .uninit          = _avf_atempo_uninit,
};

/**
 * @brief  open atempo avfilter
 * @param  [in] inst_name : the name of avfilter_t instance
 * @param  [in] atempop
 * @return NULL on error
 */
avfilter_t *avf_atempo_open(const char *inst_name, const atempo_avfp_t *atempop)
{
    int rc;
    atempo_t *atempo;
    struct avfilter *f        = NULL;
    struct avfilter_atempo *dobj = NULL;

    CHECK_PARAM(inst_name && atempop, NULL);
    if (!(sf_get_rate(atempop->sf) && (sf_get_bit(atempop->sf) == 16) && sf_get_signed(atempop->sf)
          && (sf_get_channel(atempop->sf) == 1) && atempop->speed > 0)) {
        LOGE(TAG, "atempo param failed");
        return NULL;
    }
    dobj = av_zalloc(sizeof(struct avfilter_atempo));
    f         = (avfilter_t*)dobj;
    f->ops    = &avf_ops_atempo;
    f->dobj   = dobj;
    f->bypass = atempop->speed == 1 ? 1 : 0;

    rc = avf_init(inst_name, f);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = f->ops->init ? f->ops->init((avfilter_t*)dobj) : 0;
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    atempo = atempo_new(sf_get_rate(atempop->sf));
    CHECK_RET_TAG_WITH_GOTO(atempo, err);
    atempo_set_speed(atempo, atempop->speed);

    dobj->atempo = atempo;
    dobj->sf     = atempop->sf;
    dobj->speed  = atempop->speed;
    LOGD(TAG, "open a avfilter, name = %s", f->ops->name);

    return (avfilter_t*)dobj;
err:
    avf_uninit(f);
    av_free(dobj);
    return NULL;
}


