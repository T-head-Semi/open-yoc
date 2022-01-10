/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avfilter/avfilter_swr.h"
#include "avfilter/avfilter_atempo2.h"

#define TAG                   "filter_atempo2"

struct avfilter_atempo2 {
    struct avfilter           f;

    sf_t                      sf;

    avfilter_t                *avfc;          ///< sf->swr_sf
    avfilter_t                *avf_swr1;      ///< sf->swr_sf
    avfilter_t                *avf_swr2;      ///< swr_sf->sf
    avfilter_t                *avf_atempo;    ///< real atempoer
    sf_t                      swr_sf;         ///< 16k
};

static int _avf_atempo2_init(avfilter_t *avf)
{
    //TODO
    return 0;
}

static int _avf_atempo2_control(avfilter_t *avf, int cmd, void *arg, size_t *arg_size)
{
    int rc = -1;
    struct avfilter_atempo2 *dobj = (struct avfilter_atempo2*)avf;

    switch (cmd) {
    case AVF_CMD_ATEMPO_SET_SPEED: {
        if (arg) {
            float speed = *((float*)arg);

            rc = avf_control(dobj->avf_atempo, cmd, arg, arg_size);
            if (rc == 0) {
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

static int _avf_atempo2_filter_frame(avfilter_t *avf, const avframe_t *in, avframe_t *out)
{
    int rc = -1;
    struct avfilter_atempo2 *dobj = (struct avfilter_atempo2*)avf;

    if (in->sf == dobj->sf) {
        rc = avf_filter_frame(dobj->avfc, in, out);
        if (rc <= 0) {
            //LOGD(TAG, "inner avfc filter may be err, rc = %d", rc);
            return rc;
        }

        rc = out->nb_samples;
    }

    return rc;
}

static int _avf_atempo2_uninit(avfilter_t *avf)
{
    struct avfilter_atempo2 *dobj = (struct avfilter_atempo2*)avf;

    avf_chain_close(dobj->avfc);

    return 0;
}

const struct avfilter_ops avf_ops_atempo2 = {
    .name            = "atempo2",
    .type            = AVMEDIA_TYPE_AUDIO,

    .init            = _avf_atempo2_init,
    .control         = _avf_atempo2_control,
    .filter_frame    = _avf_atempo2_filter_frame,
    .uninit          = _avf_atempo2_uninit,
};

/**
 * @brief  open atempo2 avfilter(down to 16k for freq reduce)
 * @param  [in] inst_name : the name of avfilter_t instance
 * @param  [in] atempop
 * @return NULL on error
 */
avfilter_t *avf_atempo2_open(const char *inst_name, const atempo_avfp_t *atempop)
{
    int rc;
    struct avfilter *f        = NULL;
    struct avfilter_atempo2 *dobj = NULL;

    CHECK_PARAM(inst_name && atempop, NULL);
    if (!(sf_get_rate(atempop->sf) && (sf_get_bit(atempop->sf) == 16) && sf_get_signed(atempop->sf)
          && (sf_get_channel(atempop->sf) == 1) && atempop->speed > 0)) {
        LOGE(TAG, "atempo2 param failed");
        return NULL;
    }
    dobj = av_zalloc(sizeof(struct avfilter_atempo2));
    f         = (avfilter_t*)dobj;
    f->ops    = &avf_ops_atempo2;
    f->dobj   = dobj;
    f->bypass = atempop->speed == 1 ? 1 : 0;

    rc = avf_init(inst_name, f);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = f->ops->init ? f->ops->init((avfilter_t*)dobj) : 0;
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    if (sf_get_rate(atempop->sf) > 16000) {
        swr_avfp_t swrp;
        atempo_avfp_t iatempop;

        dobj->swr_sf     = sf_make_channel(1) | sf_make_rate(16000) | sf_make_bit(16) | sf_make_signed(1);
        swrp.osf         = dobj->swr_sf;
        swrp.isf         = atempop->sf;
        dobj->avf_swr1   = avf_swr_open("atempo_swr1", &swrp);
        CHECK_RET_TAG_WITH_GOTO(dobj->avf_swr1, err);
        dobj->avfc       = dobj->avf_swr1;

        iatempop.sf      = dobj->swr_sf;
        iatempop.speed   = atempop->speed;
        dobj->avf_atempo = avf_atempo_open("atempo", &iatempop);
        CHECK_RET_TAG_WITH_GOTO(dobj->avf_atempo, err);
        avf_link_tail(dobj->avfc, dobj->avf_atempo);

        swrp.isf         = dobj->swr_sf;
        swrp.osf         = atempop->sf;
        dobj->avf_swr2   = avf_swr_open("atempo_swr2", &swrp);
        CHECK_RET_TAG_WITH_GOTO(dobj->avf_swr2, err);
        avf_link_tail(dobj->avfc, dobj->avf_swr2);
    } else {
        dobj->avf_atempo = avf_atempo_open("atempo", atempop);
        CHECK_RET_TAG_WITH_GOTO(dobj->avf_atempo, err);
        dobj->avfc = dobj->avf_atempo;
    }

    dobj->sf     = atempop->sf;
    LOGD(TAG, "open a avfilter, name = %s", f->ops->name);

    return (avfilter_t*)dobj;
err:
    avf_close(dobj->avf_swr1);
    avf_close(dobj->avf_swr2);
    avf_close(dobj->avf_atempo);
    avf_uninit(f);
    av_free(dobj);
    return NULL;
}


