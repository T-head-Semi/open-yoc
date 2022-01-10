/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_ADPCM_MS) && CONFIG_DECODER_ADPCM_MS

#include "avutil/common.h"
#include "avutil/misc.h"
#include "avformat/avformat_utils.h"
#include "avcodec/ad_cls.h"

#define TAG                    "ad_adpcm"

static int16_t _adapt_ms_table[] = {
    230, 230, 230, 230, 307, 409, 512, 614,
    768, 614, 512, 409, 307, 230, 230, 230
};

static int16_t _adapt_ms_coef1[] = { 256, 512, 0, 192, 240, 460, 392 };
static int16_t _adapt_ms_coef2[] = { 0, -256, 0, 64, 0, -208, -232 };

/* adpcm block header */
struct adpcm_bh {
    int                      coef1;
    int                      coef2;
    int                      delta;
    int                      sample1;
    int                      sample2;
};

struct ad_adpcm_priv {
    uint32_t                 block_align;    ///< size of one frame, if needed
    struct adpcm_bh          bh[2];          ///< adpcm block header. max channel: 2
};

static int16_t _adpcm_ms_update(struct adpcm_bh *h, int nibble)
{
    int predictor;

    predictor  = (((h->sample1) * (h->coef1)) + ((h->sample2) * (h->coef2))) / 256;
    predictor += ((nibble & 0x08) ? (nibble - 0x10) : nibble) * h->delta;

    h->sample2 = h->sample1;
    h->sample1 = clip_int16(predictor);
    h->delta   = (_adapt_ms_table[nibble] * h->delta) >> 8;
    h->delta   = h->delta < 16 ? 16 : h->delta;
    h->delta   = h->delta > INT_MAX / 768 ? INT_MAX / 768 : h->delta;

    return h->sample1;
}

static int _ad_adpcm_open(ad_cls_t *o)
{
    struct ad_adpcm_priv *priv = NULL;

    if (!(o->ash.block_align && sf_get_channel(o->ash.sf) <= 2)) {
        LOGE(TAG, "adpcm, block_align or channel is invalid, %u, %d", o->ash.block_align, sf_get_channel(o->ash.sf));
        return -1;
    }
    priv = av_zalloc(sizeof(struct ad_adpcm_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    o->priv = priv;
    return 0;
}

static int _ad_adpcm_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    sf_t sf;
    bio_t bio;
    int rc, channel, i, st;
    int16_t *samples;
    uint8_t predictor, nibble;
    size_t buf_size, nb_samples;
    struct ad_adpcm_priv *priv = o->priv;

    sf                = o->ash.sf;
    channel           = sf_get_channel(sf);
    buf_size          = pkt->len > o->ash.block_align ? o->ash.block_align : pkt->len;
    nb_samples        = (buf_size - 6 * channel) * 2 / channel;
    frame->sf         = sf;
    frame->nb_samples = nb_samples;
    rc = avframe_get_buffer(frame);
    if (rc < 0) {
        LOGD(TAG, "avframe_get_buffer failed, may be oom. sf = %u, ch = %d", sf, channel);
        goto quit;
    }

    st = channel > 1 ? 1 : 0;
    samples = (int16_t*)frame->data[0];
    bio_init(&bio, pkt->data, buf_size);
    predictor = bio_r8(&bio);
    if (predictor > 6) {
        LOGD(TAG, "predictor is invalid, %u.", predictor);
        goto quit;
    }
    priv->bh[0].coef1 = _adapt_ms_coef1[predictor];
    priv->bh[0].coef2 = _adapt_ms_coef2[predictor];
    if (st) {
        predictor = bio_r8(&bio);
        priv->bh[1].coef1 = _adapt_ms_coef1[predictor];
        priv->bh[1].coef2 = _adapt_ms_coef2[predictor];
    }

    priv->bh[0].delta = sign_extend(bio_r16le(&bio), 16);
    if (st)
        priv->bh[1].delta = sign_extend(bio_r16le(&bio), 16);

    priv->bh[0].sample1 = sign_extend(bio_r16le(&bio), 16);
    if (st)
        priv->bh[1].sample1 = sign_extend(bio_r16le(&bio), 16);
    priv->bh[0].sample2 = sign_extend(bio_r16le(&bio), 16);
    if (st)
        priv->bh[1].sample2 = sign_extend(bio_r16le(&bio), 16);

    *samples++ = priv->bh[0].sample2;
    if (st)
        *samples++ = priv->bh[1].sample2;
    *samples++ = priv->bh[0].sample1;
    if (st)
        *samples++ = priv->bh[1].sample1;

    i = st ? (nb_samples - 2) : (nb_samples - 2) / 2;
    for(; i > 0; i--) {
        nibble = bio_r8(&bio);
        *samples++ = _adpcm_ms_update(&priv->bh[0], nibble >> 4);
        *samples++ = _adpcm_ms_update(&priv->bh[st], nibble & 0x0F);
    }

    rc = buf_size;
    *got_frame = 1;

quit:
    return rc;
}

static int _ad_adpcm_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_adpcm_reset(ad_cls_t *o)
{
    //TODO
    return 0;
}

static int _ad_adpcm_close(ad_cls_t *o)
{
    struct ad_adpcm_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;

    return 0;
}

const struct ad_ops ad_ops_adpcm_ms = {
    .name           = "adpcm_ms",
    .id             = AVCODEC_ID_ADPCM_MS,

    .open           = _ad_adpcm_open,
    .decode         = _ad_adpcm_decode,
    .control        = _ad_adpcm_control,
    .reset          = _ad_adpcm_reset,
    .close          = _ad_adpcm_close,
};

#endif
