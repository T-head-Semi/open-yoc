/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_AMRWB) && CONFIG_DECODER_AMRWB

#include "avutil/common.h"
#include "avformat/amr_rw.h"
#include "avcodec/ad_cls.h"
#include "amrwb/pvamrwbdecoder.h"

#define TAG                    "ad_amrwb"
#define AMRWB_ONE_FRAMES       (AMR_FRAME_INTERVAL * AMRWB_RATE / 1000)
#define AMRWB_DEC_OBUF_SIZE    (AMRWB_ONE_FRAMES * sizeof(int16_t))
#define AMRWB_MIME_SIZE_MAX    (477 * sizeof(int16_t))  ///< mime unsorting data unit size max

const uint32_t wb_framesizes[] = { 17, 23, 32, 36, 40, 46, 50, 58, 60 };

struct ad_amrwb_priv {
    void                     *hdl;
    int16_t                  *obuf;      // buf for output
    void                     *dbuf;      // buf for decoder
    void                     *mime_buf;  // mime unsorting data unit
    int16_t                  *cookie;
};

static int _ad_amrwb_open(ad_cls_t *o)
{
    uint32_t msize;
    struct ad_amrwb_priv *priv = NULL;
    void *hdl = NULL, *dbuf = NULL;
    void *obuf = NULL, *mime_buf = NULL;

    priv = av_zalloc(sizeof(struct ad_amrwb_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    msize    = pvDecoder_AmrWbMemRequirements();
    dbuf     = av_malloc(msize);
    obuf     = av_malloc(AMRWB_DEC_OBUF_SIZE);
    mime_buf = av_malloc(AMRWB_MIME_SIZE_MAX);
    CHECK_RET_TAG_WITH_GOTO(dbuf && obuf && mime_buf, err);

    pvDecoder_AmrWb_Init(&hdl, dbuf, &priv->cookie);

    priv->obuf     = obuf;
    priv->dbuf     = dbuf;
    priv->mime_buf = mime_buf;
    priv->hdl      = hdl;
    o->priv        = priv;
    return 0;
err:
    av_free(obuf);
    av_free(dbuf);
    av_free(mime_buf);
    av_free(priv);
    return -1;
}

static int _ad_amrwb_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    sf_t sf;
    uint8_t mode;
    int16 ftype, fmode;
    RX_State_wb rx_state;
    int16_t nb_osamples;
    int i, rc = -1, channel = 1, bits = 16, framesize;
    struct ad_amrwb_priv *priv = o->priv;

    mode = (pkt->data[0] >> 3) & 0x0f;
    if (mode >= 9) {
        LOGE(TAG, "mode = %u, data[0] = %u.", mode, pkt->data[0]);
        goto quit;
    }

    framesize = wb_framesizes[mode];
    if (pkt->len < (framesize + 1)) {
        LOGE(TAG, "framesize = %d, pkt len = %u.", framesize + 1, pkt->len);
        goto quit;
    }

    fmode = mode;
    mime_unsorting(&pkt->data[1], priv->mime_buf, &ftype, &fmode, 1, &rx_state);
    pvDecoder_AmrWb(fmode, priv->mime_buf, priv->obuf, &nb_osamples, priv->dbuf, ftype, priv->cookie);
    if (nb_osamples != AMRWB_ONE_FRAMES) {
        LOGE(TAG, "Decoder encountered error, nb_osamples = %d, one frames = %d.", nb_osamples, AMRWB_ONE_FRAMES);
        goto quit;
    }
    for (i = 0; i < AMRWB_ONE_FRAMES; ++i) {
        priv->obuf[i] &= 0xfffC;
    }

    sf                = sf_make_channel(channel) | sf_make_rate(AMRWB_RATE) | sf_make_bit(bits) | sf_make_signed(bits > 8);
    frame->sf         = sf;
    o->ash.sf         = sf;
    frame->nb_samples = AMRWB_ONE_FRAMES;
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

static int _ad_amrwb_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_amrwb_reset(ad_cls_t *o)
{
    struct ad_amrwb_priv *priv = o->priv;

    pvDecoder_AmrWb_Reset(priv->dbuf, 1);

    return 0;
}

static int _ad_amrwb_close(ad_cls_t *o)
{
    struct ad_amrwb_priv *priv = o->priv;

    av_free(priv->dbuf);
    av_free(priv->obuf);
    av_free(priv->mime_buf);
    av_free(priv);
    o->priv = NULL;

    return 0;
}

const struct ad_ops ad_ops_amrwb = {
    .name           = "amrwb",
    .id             = AVCODEC_ID_AMRWB,

    .open           = _ad_amrwb_open,
    .decode         = _ad_amrwb_decode,
    .control        = _ad_amrwb_control,
    .reset          = _ad_amrwb_reset,
    .close          = _ad_amrwb_close,
};

#endif


