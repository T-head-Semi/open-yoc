/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_MULAW) && CONFIG_DECODER_MULAW

#include "avutil/common.h"
#include "avcodec/ad_cls.h"

#define TAG                    "ad_mulaw"

struct ad_mulaw_priv {
    //TODO:
    size_t                   frame_size;
};

static int _ad_mulaw_open(ad_cls_t *o)
{
    o->priv = av_zalloc(sizeof(struct ad_mulaw_priv));

    return o->priv ? 0 : -1;
}

static void _mulaw_decode(int16_t *out, const uint8_t *in, size_t size)
{
    int16_t x, t;

    while (size > 0) {
        size--;
        x = *in++;
        x = ~x;

        t   = ((x & 0xf) << 3) + 0x84;
        t <<= ((unsigned)x & 0x70) >> 4;

        *out++ = (x & 0x80) ? (0x84 - t) : (t - 0x84);
    }
}

static int _ad_mulaw_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    int16_t *output;
    int rc, ret = -1, size;
    sf_t sf = o->ash.sf;

    size              = pkt->len / sf_get_channel(sf);
    frame->sf         = sf;
    frame->nb_samples = size;
    rc = avframe_get_buffer(frame);
    if (rc < 0) {
        LOGE(TAG, "avframe_get_buffer failed, may be oom. sf = %s, samples = %d", sf_get_format_str(sf), frame->nb_samples);
        goto quit;
    }

    size   *= sf_get_channel(sf);
    output  = (int16_t *)frame->data[0];
    _mulaw_decode(output, pkt->data, pkt->len / sf_get_channel(sf) * sf_get_channel(sf));

    ret        = size;
    *got_frame = 1;

quit:
    return ret;
}

static int _ad_mulaw_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_mulaw_reset(ad_cls_t *o)
{
    //TODO
    return 0;
}

static int _ad_mulaw_close(ad_cls_t *o)
{
    struct ad_mulaw_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;

    return 0;
}

const struct ad_ops ad_ops_mulaw = {
    .name           = "mulaw",
    .id             = AVCODEC_ID_MULAW,

    .open           = _ad_mulaw_open,
    .decode         = _ad_mulaw_decode,
    .control        = _ad_mulaw_control,
    .reset          = _ad_mulaw_reset,
    .close          = _ad_mulaw_close,
};

#endif

