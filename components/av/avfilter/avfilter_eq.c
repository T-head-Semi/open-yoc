/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avfilter/avfilter_eq.h"
#include "aef/eq.h"

#define TAG                   "filter_eq"

struct avfilter_eq {
    struct avfilter           f;

    sf_t                      sf;
    uint8_t                   enable;
    uint8_t                   eq_segments;
    eqx_t                     *eq;
};

static int _avf_eq_control(avfilter_t *avf, int cmd, void *arg, size_t *arg_size)
{
    int rc = -1;
    struct avfilter_eq *dobj = (struct avfilter_eq*)avf;

    switch (cmd) {
    case AVF_CMD_EQ_SET_ENABLE: {
        eq_seten_t *avfp = arg;
        rc = eqx_set_enable(dobj->eq, avfp->enable);
    }
    break;
    case AVF_CMD_EQ_SET_PARAM: {
        eq_setpa_t *avfp = arg;
        rc = eqx_set_param(dobj->eq, avfp->segid, &avfp->param);
    }
    break;
    default:
        break;
    }

    return rc;
}

static int _avf_eq_filter_frame(avfilter_t *avf, const avframe_t *in, avframe_t *out)
{
    int rc = -1;
    struct avfilter_eq *dobj = (struct avfilter_eq*)avf;
    eqx_t *eq                = dobj->eq;

    if (eq && (in->sf == dobj->sf)) {
        out->sf         = dobj->sf;
        out->nb_samples = in->nb_samples;
        rc = avframe_get_buffer(out);
        if (rc < 0) {
            LOGE(TAG, "may be oom, rc = %d", rc);
            return -1;
        }
        rc = eqx_process(eq, (const int16_t*)in->data[0], (int16_t*)out->data[0], in->nb_samples);
        CHECK_RET_TAG_WITH_RET(rc == 0, -1);

        rc = out->nb_samples;
    }

    return rc;
}

static int _avf_eq_uninit(avfilter_t *avf)
{
    struct avfilter_eq *dobj = (struct avfilter_eq*)avf;

    eqx_free(dobj->eq);
    return 0;
}

const struct avfilter_ops avf_ops_eq = {
    .name            = "eq",
    .type            = AVMEDIA_TYPE_AUDIO,

    .control         = _avf_eq_control,
    .filter_frame    = _avf_eq_filter_frame,
    .uninit          = _avf_eq_uninit,
};

/**
 * @brief  open eq avfilter
 * @param  [in] inst_name : the name of avfilter_t instance
 * @param  [in] eqp
 * @return NULL on error
 */
avfilter_t *avf_eq_open(const char *inst_name, const eq_avfp_t *eqp)
{
    int rc;
    eqx_t *eq;
    struct avfilter *f        = NULL;
    struct avfilter_eq *dobj = NULL;

    CHECK_PARAM(inst_name && eqp, NULL);
    if (!(sf_get_rate(eqp->sf) && (sf_get_bit(eqp->sf) == 16) && sf_get_signed(eqp->sf)
          && (sf_get_channel(eqp->sf) == 1) && eqp->eq_segments)) {
        LOGE(TAG, "eq param failed");
        return NULL;
    }
    dobj = av_zalloc(sizeof(struct avfilter_eq));
    CHECK_RET_TAG_WITH_RET(dobj, NULL);
    f       = (avfilter_t*)dobj;
    f->ops  = &avf_ops_eq;
    f->dobj = dobj;

    rc = avf_init(inst_name, f);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = f->ops->init ? f->ops->init((avfilter_t*)dobj) : 0;
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    eq = eqx_new(sf_get_rate(eqp->sf), eqp->eq_segments);
    CHECK_RET_TAG_WITH_GOTO(eq, err);
    dobj->eq          = eq;
    dobj->sf          = eqp->sf;
    dobj->eq_segments = eqp->eq_segments;
    LOGD(TAG, "open a avfilter, name = %s", f->ops->name);

    return (avfilter_t*)dobj;
err:
    avf_uninit(f);
    av_free(dobj);
    return NULL;
}


