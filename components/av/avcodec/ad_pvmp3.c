/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_PVMP3) && CONFIG_DECODER_PVMP3

#include "avutil/common.h"
#include "avcodec/ad_cls.h"
#include "pvmp3dec/pvmp3decoder_api.h"

#define TAG                    "ad_pvmp3"
#define PVMP3DEC_OBUF_SIZE     (4608 * 1.2)

struct ad_pvmp3_priv {
    void                     *obuf;  // buf for output
    void                     *dbuf;  // buf for decoder
    tPVMP3DecoderExternal    config;
};

static int _ad_pvmp3_open(ad_cls_t *o)
{
    uint32_t msize;
    void *dbuf = NULL, *obuf = NULL;
    tPVMP3DecoderExternal *config;
    struct ad_pvmp3_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct ad_pvmp3_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    config = &priv->config;
    msize  = pvmp3_decoderMemRequirements();
    dbuf   = av_malloc(msize);
    obuf   = av_malloc(PVMP3DEC_OBUF_SIZE);
    CHECK_RET_TAG_WITH_GOTO(dbuf && obuf, err);

    pvmp3_InitDecoder(config, dbuf);
    config->crcEnabled    = false;
    config->equalizerType = flat;
    config->pOutputBuffer = obuf;

    priv->obuf = obuf;
    priv->dbuf = dbuf;
    o->priv    = priv;
    return 0;

err:
    av_free(obuf);
    av_free(dbuf);
    av_free(priv);
    return -1;
}

static int _ad_pvmp3_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    sf_t sf;
    int ret = -1;
    ERROR_CODE ecode;
    tPVMP3DecoderExternal *config;
    int rc, channel, bits = 16;
    struct ad_pvmp3_priv *priv = o->priv;

    config = &priv->config;
    config->inputBufferMaxLength     = 0;
    config->inputBufferUsedLength    = 0;
    config->pInputBuffer             = pkt->data;
    config->inputBufferCurrentLength = pkt->len;
    config->outputFrameSize          = PVMP3DEC_OBUF_SIZE / sizeof(int16_t);

    ecode = pvmp3_framedecoder(config, priv->dbuf);
    if (!((ecode == NO_DECODING_ERROR) &&
          config->outputFrameSize && config->num_channels && config->samplingRate)) {
        LOGE(TAG, "ecode = %d, frame size = %d, ch = %d, rate = %d.", ecode,
             config->outputFrameSize, config->num_channels, config->samplingRate);
        goto quit;
    }

    channel           = config->num_channels;
    sf                = sf_make_channel(channel) | sf_make_rate(config->samplingRate) | sf_make_bit(bits) | sf_make_signed(bits > 8);
    frame->sf         = sf;
    o->ash.sf         = sf;
    frame->nb_samples = config->outputFrameSize / channel ;

    rc = avframe_get_buffer(frame);
    if (rc < 0) {
        LOGD(TAG, "avframe_get_buffer failed, may be oom. sf = %u, ch = %d, rate = %d", sf, channel,
             config->samplingRate);
        ret = -1;
        goto quit;
    }

    rc = sf_get_frame_size(sf) * frame->nb_samples;
    memcpy(frame->data[0], config->pOutputBuffer, rc);
    ret = config->inputBufferUsedLength;
    *got_frame = 1;

quit:
    return ret;
}

static int _ad_pvmp3_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_pvmp3_reset(ad_cls_t *o)
{
    struct ad_pvmp3_priv *priv = o->priv;

    pvmp3_resetDecoder(priv->dbuf);

    return 0;
}

static int _ad_pvmp3_close(ad_cls_t *o)
{
    struct ad_pvmp3_priv *priv = o->priv;

    av_free(priv->obuf);
    av_free(priv->dbuf);
    av_free(priv);
    o->priv = NULL;

    return 0;
}

const struct ad_ops ad_ops_pvmp3 = {
    .name           = "pvmp3dec",
    .id             = AVCODEC_ID_MP3,

    .open           = _ad_pvmp3_open,
    .decode         = _ad_pvmp3_decode,
    .control        = _ad_pvmp3_control,
    .reset          = _ad_pvmp3_reset,
    .close          = _ad_pvmp3_close,
};

#endif
