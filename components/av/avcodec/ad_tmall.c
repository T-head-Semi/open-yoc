/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_TMALL) && CONFIG_DECODER_TMALL

#include "avutil/common.h"
#include "avcodec/ad_cls.h"
#include "ad_tmall.h"

#define TAG                    "ad_tmall"

#define ADTMALL_FRAME_POOL_SIZE (5*1024)  // 1152 samples per frame max for mp3
struct ad_tmall_priv {
    void            *hdl;
    uint8_t         *pool;
    size_t          pool_size;
};

static int _ad_tmall_open(ad_cls_t *o)
{
    int format;
    uint8_t *pool = NULL;
    struct ad_tmall_priv *priv = NULL;
    void *hdl;

    priv = av_zalloc(sizeof(struct ad_tmall_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    pool = av_zalloc(ADTMALL_FRAME_POOL_SIZE);
    CHECK_RET_TAG_WITH_GOTO(pool, err);

    if (o->ash.id == AVCODEC_ID_MP3) {
        format = AG_AUDIO_FORMAT_MP3;
    } else if (o->ash.id == AVCODEC_ID_AAC) {
        format = AG_AUDIO_FORMAT_AAC;
    } else {
        LOGE(TAG, "%s, %d faild. id = %d", __FUNCTION__, __LINE__, o->ash.id);
        goto err;
    }

    //hdl = ad_adapter_open(format);
    hdl = ad_adapter_open2(format, o->ash.extradata, o->ash.extradata_size);
    CHECK_RET_TAG_WITH_GOTO(hdl, err);

    o->priv         = priv;
    priv->hdl       = hdl;
    priv->pool      = pool;
    priv->pool_size = ADTMALL_FRAME_POOL_SIZE;

    return 0;
err:
    av_free(priv);

    return -1;
}

static int _ad_tmall_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    int rc;
    sf_t sf;
    uint8_t *odata;
    int osize, channel, osample;
    struct ad_tmall_priv *priv = o->priv;

    if (!(frame->mpool && frame->msize))
        avframe_set_mempool(frame, priv->pool, priv->pool_size);

    odata = frame->mpool;
    osize = frame->msize;
    rc = ad_adapter_decoder(priv->hdl, pkt->data, pkt->len, odata, &osize, &osample);
    if (rc > 0) {
        //FIXME: aac may be error.
        sf                 = o->ash.sf;
        channel            = sf_get_channel(sf);
        frame->sf          = sf;
        frame->nb_samples  = osample / channel;
        frame->data[0]     = priv->pool;
        frame->linesize[0] = osize;
        *got_frame         = 1;
    } else {
        /* FIXME: patch for tmall special require */
        rc = (rc == -100) ? 0 : rc;
    }


    return rc >= 0 ? pkt->len : -1;
}

static int _ad_tmall_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_tmall_reset(ad_cls_t *o)
{
    int rc;
    struct ad_tmall_priv *priv = o->priv;

    rc = ad_adapter_reset(priv->hdl);

    return rc;
}

static int _ad_tmall_close(ad_cls_t *o)
{
    struct ad_tmall_priv *priv = o->priv;

    ad_adapter_close(priv->hdl);
    av_free(priv->pool);

    av_free(priv);
    o->priv = NULL;
    return 0;
}

// test for tmall
const struct ad_ops ad_ops_tmall = {
    .name           = "tmall",
    .id             = AVCODEC_ID_AAC | AVCODEC_ID_MP3,

    .open           = _ad_tmall_open,
    .decode         = _ad_tmall_decode,
    .control        = _ad_tmall_control,
    .reset          = _ad_tmall_reset,
    .close          = _ad_tmall_close,
};

#endif

