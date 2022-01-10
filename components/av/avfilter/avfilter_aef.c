/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avfilter/avfilter_aef.h"
#include "aef/aef.h"

#define TAG                   "filter_aef"

struct avfilter_aef {
    struct avfilter           f;

    sf_t                      sf;
    uint8_t                   *conf;
    size_t                    conf_size;
    size_t                    nsamples_max;   ///< the max frame len for input on processing
    aefx_t                    *aef;
};

static int _avf_aef_init(avfilter_t *avf)
{
    //TODO
    return 0;
}

static int _avf_aef_control(avfilter_t *avf, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _avf_aef_filter_frame(avfilter_t *avf, const avframe_t *in, avframe_t *out)
{
    int rc = -1;
    struct avfilter_aef *dobj = (struct avfilter_aef*)avf;
    aefx_t *aef               = dobj->aef;

    if (aef && (in->sf == dobj->sf)) {
        out->sf         = dobj->sf;
        out->nb_samples = in->nb_samples;
        rc = avframe_get_buffer(out);
        if (rc < 0) {
            LOGE(TAG, "may be oom, rc = %d", rc);
            return -1;
        }
        rc = aefx_process(aef, (const int16_t*)in->data[0], (int16_t*)out->data[0], in->nb_samples);
        CHECK_RET_TAG_WITH_RET(rc == 0, -1);

        rc = out->nb_samples;
    }

    return rc;
}

static int _avf_aef_uninit(avfilter_t *avf)
{
    struct avfilter_aef *dobj = (struct avfilter_aef*)avf;

    aefx_free(dobj->aef);
    return 0;
}

const struct avfilter_ops avf_ops_aef = {
    .name            = "aef",
    .type            = AVMEDIA_TYPE_AUDIO,

    .init            = _avf_aef_init,
    .control         = _avf_aef_control,
    .filter_frame    = _avf_aef_filter_frame,
    .uninit          = _avf_aef_uninit,
};

/**
 * @brief  open aef avfilter
 * @param  [in] inst_name : the name of avfilter_t instance
 * @param  [in] aefp
 * @return NULL on error
 */
avfilter_t *avf_aef_open(const char *inst_name, const aef_avfp_t *aefp)
{
    int rc;
    aefx_t *aef;
    struct avfilter *f        = NULL;
    struct avfilter_aef *dobj = NULL;

    CHECK_PARAM(inst_name && aefp, NULL);
    if (!(sf_get_rate(aefp->sf) && (sf_get_bit(aefp->sf) == 16) && sf_get_signed(aefp->sf)
          && (sf_get_channel(aefp->sf) == 1) && aefp->nsamples_max)) {
        LOGE(TAG, "aef param failed");
        return NULL;
    }
    dobj = av_zalloc(sizeof(struct avfilter_aef));
    CHECK_RET_TAG_WITH_RET(dobj, NULL);
    f       = (avfilter_t*)dobj;
    f->ops  = &avf_ops_aef;
    f->dobj = dobj;

    rc = avf_init(inst_name, f);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = f->ops->init ? f->ops->init((avfilter_t*)dobj) : 0;
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    aef = aefx_new(sf_get_rate(aefp->sf), aefp->conf, aefp->conf_size, aefp->nsamples_max);
    CHECK_RET_TAG_WITH_GOTO(aef, err);

    dobj->aef          = aef;
    dobj->sf           = aefp->sf;
    dobj->nsamples_max = aefp->nsamples_max;
    LOGD(TAG, "open a avfilter, name = %s", f->ops->name);

    return (avfilter_t*)dobj;
err:
    avf_uninit(f);
    av_free(dobj);
    return NULL;
}


