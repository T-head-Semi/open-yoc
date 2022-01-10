/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_SPEEX) && CONFIG_DECODER_SPEEX

#include "avutil/common.h"
#include "avcodec/ad_cls.h"
#include <speex/speex.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>

#define TAG                    "ad_speex"

struct ad_speex_priv {
    size_t                   frame_size;
    void                     *dec;
    SpeexBits                bits;
    SpeexStereoState         stereo;
};

static int _ad_speex_open(ad_cls_t *o)
{
    int spx_mode;
    sf_t sf  = o->ash.sf;
    int rate = sf_get_rate(sf);
    void *dec                  = NULL;
    struct ad_speex_priv *priv = NULL;

    switch (rate) {
    case 8000:
        spx_mode = 0;
        break;
    case 16000:
        spx_mode = 1;
        break;
    case 32000:
        spx_mode = 2;
        break;
    default:
        LOGD(TAG, "not support rate for speex, rate = %d", rate);
        return -1;
    }

    priv = av_zalloc(sizeof(struct ad_speex_priv));
    dec  = speex_decoder_init(speex_lib_get_mode(spx_mode));
    CHECK_RET_TAG_WITH_GOTO(priv && dec, err);

    speex_bits_init(&priv->bits);
    if (sf_get_channel(sf) == 2) {
        SpeexCallback callback;

        memset(&callback, 0, sizeof(SpeexCallback));
        callback.data        = &priv->stereo;
        callback.func        = speex_std_stereo_request_handler;
        callback.callback_id = SPEEX_INBAND_STEREO;

        priv->stereo = (SpeexStereoState)SPEEX_STEREO_STATE_INIT;
        speex_decoder_ctl(dec, SPEEX_SET_HANDLER, &callback);
    }

    priv->dec        = dec;
    priv->frame_size = 160 << spx_mode;
    o->priv          = priv;
    return 0;
err:
    if (priv) {
        speex_bits_destroy(&priv->bits);
        av_free(priv);
    }
    if (dec)
        speex_decoder_destroy(dec);
    return -1;
}

static int _ad_speex_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    int rc, ret = -1;
    int16_t *output;
    sf_t sf = o->ash.sf;
    struct ad_speex_priv *priv = o->priv;

    frame->sf         = sf;
    frame->nb_samples = priv->frame_size;
    rc = avframe_get_buffer(frame);
    if (rc < 0) {
        LOGE(TAG, "avframe_get_buffer failed, may be oom. sf = %s, samples = %d", sf_get_format_str(sf), frame->nb_samples);
        goto quit;
    }

    output = (int16_t *)frame->data[0];
    speex_bits_reset(&priv->bits);
    speex_bits_read_from(&priv->bits, (const char*)pkt->data, pkt->len);
    rc = speex_decode_int(priv->dec, &priv->bits, output);
    if (rc < 0) {
        LOGD(TAG, "decode speex frame fail");
        goto quit;
    }
    if (sf_get_channel(sf) == 2)
        speex_decode_stereo_int(output, priv->frame_size, &priv->stereo);

    ret        = pkt->len;
    *got_frame = 1;

quit:
    return ret;
}

static int _ad_speex_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_speex_reset(ad_cls_t *o)
{
    //TODO
    return 0;
}

static int _ad_speex_close(ad_cls_t *o)
{
    struct ad_speex_priv *priv = o->priv;

    speex_decoder_destroy(priv->dec);
    speex_bits_destroy(&priv->bits);
    av_free(priv);
    o->priv = NULL;

    return 0;
}

const struct ad_ops ad_ops_speex = {
    .name           = "speex",
    .id             = AVCODEC_ID_SPEEX,

    .open           = _ad_speex_open,
    .decode         = _ad_speex_decode,
    .control        = _ad_speex_control,
    .reset          = _ad_speex_reset,
    .close          = _ad_speex_close,
};

#endif
