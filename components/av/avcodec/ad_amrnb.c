/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_AMRNB) && CONFIG_DECODER_AMRNB

#include "avutil/common.h"
#include "avformat/amr_rw.h"
#include "avcodec/ad_cls.h"
#include "amrnb/frame_type_3gpp.h"
#include "amrnb/gsmamr_dec.h"

#define TAG                    "ad_amrnb"
#define AMRNB_ONE_FRAMES       (AMR_FRAME_INTERVAL * AMRNB_RATE / 1000)
#define AMRNB_DEC_OBUF_SIZE    (AMRNB_ONE_FRAMES * sizeof(int16_t))

const uint32_t nb_framesizes[] = { 12, 13, 15, 17, 19, 20, 26, 31 };

struct ad_amrnb_priv {
    void                     *hdl;
    void                     *obuf;  // buf for output
};

static int _ad_amrnb_open(ad_cls_t *o)
{
    int rc;
    void *hdl = NULL;
    void *obuf = NULL;
    struct ad_amrnb_priv *priv = NULL;

    obuf = av_malloc(AMRNB_DEC_OBUF_SIZE);
    priv = av_zalloc(sizeof(struct ad_amrnb_priv));
    CHECK_RET_TAG_WITH_GOTO(priv && obuf, err);

    rc = GSMInitDecode(&hdl, (Word8*)"AMRNBDecoder");
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    priv->obuf = obuf;
    priv->hdl  = hdl;
    o->priv    = priv;
    return 0;
err:
    GSMDecodeFrameExit(&hdl);
    av_free(obuf);
    av_free(priv);
    return -1;
}

static int _ad_amrnb_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    sf_t sf;
    enum Frame_Type_3GPP mode;
    int rc = -1, channel = 1, bits = 16, framesize;
    struct ad_amrnb_priv *priv = o->priv;
    void *hdl = priv->hdl;

    mode = (enum Frame_Type_3GPP)((pkt->data[0] >> 3) & 0x0f);
    if (mode >= AMR_SID) {
        LOGE(TAG, "mode = %u, data[0] = %u.", mode, pkt->data[0]);
        goto quit;
    }

    framesize = nb_framesizes[mode];
    if (pkt->len < (framesize + 1)) {
        LOGE(TAG, "framesize = %d, pkt len = %u.", framesize + 1, pkt->len);
        goto quit;
    }

    rc = AMRDecode(hdl, mode, &pkt->data[1], (int16_t*)priv->obuf, MIME_IETF);
    if(rc < 0) {
        LOGE(TAG, "Decoder encountered error");
        goto quit;
    }

    sf                = sf_make_channel(channel) | sf_make_rate(AMRNB_RATE) | sf_make_bit(bits) | sf_make_signed(bits > 8);
    frame->sf         = sf;
    o->ash.sf         = sf;
    frame->nb_samples = AMRNB_ONE_FRAMES;

    rc = avframe_get_buffer(frame);
    if (rc < 0) {
        LOGD(TAG, "avframe_get_buffer failed, may be oom.  nb_samples = %u, sf=> %s", frame->nb_samples, sf_get_format_str(sf));
        rc = -1;
        goto quit;
    }

    rc = sf_get_frame_size(sf) * frame->nb_samples;
    memcpy(frame->data[0], priv->obuf, rc);
    rc = framesize + 1;
    *got_frame = 1;

quit:
    return rc;
}

static int _ad_amrnb_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_amrnb_reset(ad_cls_t *o)
{
    int rc;
    struct ad_amrnb_priv *priv = o->priv;
    void *hdl = priv->hdl;

    GSMDecodeFrameExit(&hdl);
    priv->hdl = NULL;

    rc = GSMInitDecode(&hdl, (Word8*)"AMRNBDecoder");
    CHECK_RET_TAG_WITH_RET(rc == 0, -1);
    priv->hdl = hdl;

    return 0;
}

static int _ad_amrnb_close(ad_cls_t *o)
{
    struct ad_amrnb_priv *priv = o->priv;

    GSMDecodeFrameExit(&priv->hdl);
    av_free(priv->obuf);
    av_free(priv);
    o->priv = NULL;

    return 0;
}

const struct ad_ops ad_ops_amrnb = {
    .name           = "amrnb",
    .id             = AVCODEC_ID_AMRNB,

    .open           = _ad_amrnb_open,
    .decode         = _ad_amrnb_decode,
    .control        = _ad_amrnb_control,
    .reset          = _ad_amrnb_reset,
    .close          = _ad_amrnb_close,
};

#endif

