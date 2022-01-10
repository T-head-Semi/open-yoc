/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_OPUS) && CONFIG_DECODER_OPUS

#include "avutil/common.h"
#include "avcodec/ad_cls.h"
#include <opus/opus_defines.h>
#include <opus/opus.h>

#define TAG                    "ad_opus"
#define OPUSDEC_OBUF_SIZE      (48000 / 1000 * sizeof(short) * 2 * 60)

struct ad_opus_priv {
    void                     *obuf;  // buf for output
    OpusDecoder              *dec;
};

static int _ad_opus_open(ad_cls_t *o)
{
    int err;
    sf_t sf = o->ash.sf;
    void *obuf                = NULL;
    OpusDecoder *dec          = NULL;
    struct ad_opus_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct ad_opus_priv));
    dec = opus_decoder_create(sf_get_rate(sf), sf_get_channel(sf), &err);
    CHECK_RET_TAG_WITH_GOTO(priv && dec, err);

    obuf   = av_malloc(OPUSDEC_OBUF_SIZE);
    CHECK_RET_TAG_WITH_GOTO(obuf, err);

    priv->dec  = dec;
    priv->obuf = obuf;
    o->priv    = priv;
    return 0;
err:
    opus_decoder_destroy(dec);
    av_free(obuf);
    av_free(priv);
    return -1;
}

static int _ad_opus_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    int rc, ret = -1;
    sf_t sf = o->ash.sf;
    struct ad_opus_priv *priv = o->priv;

    rc = opus_decode(priv->dec, pkt->data, pkt->len, priv->obuf, OPUSDEC_OBUF_SIZE, 0);
    if (rc <= 0) {
        goto quit;
    }

    frame->sf         = sf;
    frame->nb_samples = rc;
    rc = avframe_get_buffer(frame);
    if (rc < 0) {
        LOGD(TAG, "avframe_get_buffer failed, may be oom. sf = %s, samples = %d", sf_get_format_str(sf), frame->nb_samples);
        goto quit;
    }

    ret = pkt->len;
    rc  = sf_get_frame_size(sf) * frame->nb_samples;
    memcpy(frame->data[0], priv->obuf, rc);
    *got_frame = 1;

quit:
    return ret;
}

static int _ad_opus_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_opus_reset(ad_cls_t *o)
{
    //TODO
    return 0;
}

static int _ad_opus_close(ad_cls_t *o)
{
    struct ad_opus_priv *priv = o->priv;

    opus_decoder_destroy(priv->dec);
    av_free(priv->obuf);
    av_free(priv);
    o->priv = NULL;

    return 0;
}

const struct ad_ops ad_ops_opus = {
    .name           = "opus",
    .id             = AVCODEC_ID_OPUS,

    .open           = _ad_opus_open,
    .decode         = _ad_opus_decode,
    .control        = _ad_opus_control,
    .reset          = _ad_opus_reset,
    .close          = _ad_opus_close,
};

#endif
