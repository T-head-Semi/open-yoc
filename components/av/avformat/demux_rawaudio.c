/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_RAWAUDIO) && CONFIG_DEMUXER_RAWAUDIO
#include "avutil/url_parse.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                    "demux_rawaudio"

struct rawaudio_priv {
    //TODO
    char                       avcodec[16];
    int32_t                    rate;
    int32_t                    channel;
    uint32_t                   frame_size;
    int32_t                    start_pos;
    uint64_t                   iduration;  ///< inner duration, base time_base
};

/* url example: mem://addr=765432&size=1024&avformat=mp3&acodec=mp3 */
static int _demux_rawaudio_open(demux_cls_t *o)
{
    sf_t sf = 0;
    struct rawaudio_priv *priv;

    priv = av_zalloc(sizeof(struct rawaudio_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    url_get_item_value(stream_get_url(o->s), "avcodec", priv->avcodec, sizeof(priv->avcodec));
    url_get_item_value_int(stream_get_url(o->s), "rate", &priv->rate);
    CHECK_RET_TAG_WITH_GOTO(priv->rate > 0, err);
    url_get_item_value_int(stream_get_url(o->s), "channel", &priv->channel);
    CHECK_RET_TAG_WITH_GOTO(priv->channel > 0, err);

    if (0 == strcmp(priv->avcodec, "pcm_s16be")) {
        sf = sf_make_bit(16) | sf_make_signed(1) | sf_make_bigendian(1);
    } else if (0 == strcmp(priv->avcodec, "pcm_s16le")) {
        sf = sf_make_bit(16) | sf_make_signed(1) | sf_make_bigendian(0);
    } else if (0 == strcmp(priv->avcodec, "pcm_s24be")) {
        sf = sf_make_bit(24) | sf_make_signed(1) | sf_make_bigendian(1);
    } else if (0 == strcmp(priv->avcodec, "pcm_s24le")) {
        sf = sf_make_bit(24) | sf_make_signed(1) | sf_make_bigendian(0);
    } else if (0 == strcmp(priv->avcodec, "pcm_s32be")) {
        sf = sf_make_bit(32) | sf_make_signed(1) | sf_make_bigendian(1);
    } else if (0 == strcmp(priv->avcodec, "pcm_s32le")) {
        sf = sf_make_bit(32) | sf_make_signed(1) | sf_make_bigendian(0);
    } else if (0 == strcmp(priv->avcodec, "pcm_s8")) {
        sf = sf_make_bit(8) | sf_make_signed(1);
    } else if (0 == strcmp(priv->avcodec, "pcm_u16be")) {
        sf = sf_make_bit(16) | sf_make_signed(0) | sf_make_bigendian(0);
    } else if (0 == strcmp(priv->avcodec, "pcm_u16le")) {
        sf = sf_make_bit(16) | sf_make_signed(0) | sf_make_bigendian(0);
    } else if (0 == strcmp(priv->avcodec, "pcm_u24be")) {
        sf = sf_make_bit(24) | sf_make_signed(0) | sf_make_bigendian(1);
    } else if (0 == strcmp(priv->avcodec, "pcm_u24le")) {
        sf = sf_make_bit(24) | sf_make_signed(0) | sf_make_bigendian(0);
    } else if (0 == strcmp(priv->avcodec, "pcm_u32be")) {
        sf = sf_make_bit(32) | sf_make_signed(0) | sf_make_bigendian(1);
    } else if (0 == strcmp(priv->avcodec, "pcm_u32le")) {
        sf = sf_make_bit(32) | sf_make_signed(0) | sf_make_bigendian(0);
    } else if (0 == strcmp(priv->avcodec, "pcm_u8")) {
        sf = sf_make_bit(8) | sf_make_signed(0);
    } else {
        LOGE(TAG, "wrong acodec = %s", priv->avcodec);
        goto err;
    }

    sf              |= sf_make_channel(priv->channel) | sf_make_rate(priv->rate);
    o->ash.id        = AVCODEC_ID_RAWAUDIO;
    o->ash.sf        = sf;
    priv->start_pos  = 0;
    o->time_scale    = AV_FLICK_BASE / priv->rate;
    o->duration      = 1.0 * (stream_get_size(o->s) / sf_get_frame_size(sf)) / priv->rate * 1000;
    priv->iduration  = o->duration / 1000.0 * o->time_scale;
    o->priv          = priv;
    return 0;
err:
    av_free(priv);
    return -1;
}

static int _demux_rawaudio_close(demux_cls_t *o)
{
    struct rawaudio_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _demux_rawaudio_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc;
    sf_t sf                    = o->ash.sf;
    size_t flen                = CONFIG_AV_SAMPLE_NUM_PER_FRAME_MAX * sf_get_frame_size(sf);
    struct rawaudio_priv *priv = o->priv;

    if (pkt->size < flen) {
        rc = avpacket_grow(pkt, flen);
        if (rc < 0) {
            LOGE(TAG, "avpacket resize fail. pkt->size = %d, framesize = %d", pkt->size, flen);
            return -1;
        }
    }

    rc = stream_read(o->s, pkt->data, pkt->size);
    pkt->len = (rc > 0) ? rc : 0;
    if (priv->iduration > 0) {
        pkt->pts = 1.0 * stream_tell(o->s) / stream_get_size(o->s) * priv->iduration;
    }

    return rc;
}

static int _demux_rawaudio_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    int fsize;
    size_t new_pos;
    struct rawaudio_priv *priv = o->priv;

    fsize = stream_get_size(o->s);
    if (!(timestamp < priv->iduration && fsize > 0 && priv->start_pos >= 0)) {
        LOGE(TAG, "seek failed. ts = %llu, idu = %llu, file_size = %d, spos = %d",
             timestamp, priv->iduration, fsize, priv->start_pos);
        return -1;
    }

    new_pos = (1.0 * timestamp / priv->iduration) * fsize;
    new_pos = AV_ALIGN_SIZE(new_pos, sf_get_frame_size(o->ash.sf));
    rc      = stream_seek(o->s, new_pos, SEEK_SET);

    return rc;
}

static int _demux_rawaudio_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_rawaudio = {
    .name            = "rawaudio",
    .type            = AVFORMAT_TYPE_RAWAUDIO,
    .extensions      = { "pcm", NULL },

    .open            = _demux_rawaudio_open,
    .close           = _demux_rawaudio_close,
    .read_packet     = _demux_rawaudio_read_packet,
    .seek            = _demux_rawaudio_seek,
    .control         = _demux_rawaudio_control,
};
#endif


