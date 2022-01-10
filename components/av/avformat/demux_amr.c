/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_AMR) && CONFIG_DEMUXER_AMR
#include "avformat/avformat_utils.h"
#include "avformat/amr_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                                     "demux_amr"
#define AMR_SYNC_HDR_MAX                        (2 * 1024)

struct amr_priv {
    uint32_t                   start_pos;
    uint32_t                   amr_size;
    uint64_t                   iduration;        ///< inner duration, base time_base
    uint8_t                    amr_type;
    struct amr_hdr             hinfo;
};

static int _demux_amr_read_packet(demux_cls_t *o, avpacket_t *pkt);

static int _demux_amr_probe(const avprobe_data_t *pd)
{
    if (!strncmp((const char*)pd->buf, AMRNB_STR, 6))
        return AVPROBE_SCORE_MAX;
    else if (!strncmp((const char*)pd->buf, AMRWB_STR, 9))
        return AVPROBE_SCORE_MAX;

    return 0;
}

static int _demux_amr_open(demux_cls_t *o)
{
    sf_t sf;
    int rc, rate;
    int64_t fsize;
    avcodec_id_t id;
    uint8_t str_size;
    uint8_t amr_str[9];
    struct amr_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct amr_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    o->priv = priv;
    stream_read(o->s, amr_str, 9);
    if (!strncmp((const char*)amr_str, AMRNB_STR, 6)) {
        str_size       = 6;
        id             = AVCODEC_ID_AMRNB;
        rate           = 8000;
        priv->amr_type = AMR_TYPE_NB;
        stream_seek(o->s, 6, SEEK_SET);
    } else if (!strncmp((const char*)amr_str, AMRWB_STR, 9)) {
        str_size       = 9;
        rate           = 16000;
        priv->amr_type = AMR_TYPE_WB;
        id             = AVCODEC_ID_AMRWB;
    } else {
        goto err;
    }

    priv->start_pos = stream_tell(o->s);
    rc = _demux_amr_read_packet(o, &o->fpkt);
    CHECK_RET_TAG_WITH_GOTO(rc > 0, err);

    sf            = sf_make_channel(1) | sf_make_rate(rate) | sf_make_bit(16) | sf_make_signed(1);
    fsize         = stream_get_size(o->s);
    o->ash.sf     = sf;
    o->ash.id     = id;
    o->time_scale = AV_FLICK_BASE / rate;
    if (fsize > 0 && priv->hinfo.framesize > 0) {
        priv->amr_size  = fsize - priv->start_pos;
        o->duration     = 20 * priv->amr_size / priv->hinfo.framesize;
        priv->iduration = o->duration / 1000.0 * o->time_scale;
    }

    return 0;
err:
    av_free(priv);
    o->priv = NULL;
    return -1;
}

static int _demux_amr_close(demux_cls_t *o)
{
    struct amr_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _demux_amr_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = -1;
    int len = 0, eof;
    uint8_t hdr[AMR_HDR_SIZE];
    struct amr_priv *priv = o->priv;
    uint8_t scode = priv->hinfo.scode;
    struct amr_hdr info, *hinfo;

    hinfo = scode == 0 ? &priv->hinfo : &info;
    stream_read(o->s, hdr, AMR_HDR_SIZE);
    rc = amr_sync(sync_read_stream, o->s, AMR_SYNC_HDR_MAX, hdr, priv->amr_type, hinfo);
    if (rc < 0) {
        LOGE(TAG, "amr sync failed, rc = %d", rc);
        goto err;
    }

    if (pkt->size < hinfo->framesize) {
        rc = avpacket_grow(pkt, hinfo->framesize);
        if (rc < 0) {
            LOGE(TAG, "avpacket resize fail. pkt->size = %d, fsize = %d", pkt->size, hinfo->framesize);
            goto err;
        }
    }

    memcpy(pkt->data, hdr, AMR_HDR_SIZE);
    len = stream_read(o->s, pkt->data + AMR_HDR_SIZE, hinfo->framesize - AMR_HDR_SIZE);
    if (len != hinfo->framesize - AMR_HDR_SIZE) {
        LOGE(TAG, "stream read err, len = %d, diff = %d, frame size = %d", len, hinfo->framesize - AMR_HDR_SIZE, hinfo->framesize);
        goto err;
    }
    len += AMR_HDR_SIZE;
    pkt->len = len;
    if (priv->iduration > 0 && priv->amr_size > 0 && priv->start_pos >= 0) {
        pkt->pts = 1.0 * (stream_tell(o->s) - priv->start_pos) / priv->amr_size * priv->iduration;
    }

    return len;
err:
    eof = stream_is_eof(o->s);
    rc  = eof ? 0 : rc;
    LOGI(TAG, "read packet may be eof. eof = %d, rc = %d, url = %s", eof, rc, stream_get_url(o->s));
    return rc;
}

static int _demux_amr_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    uint64_t new_pos;
    struct amr_priv *priv = o->priv;
    size_t packet_size    = priv->hinfo.framesize;

    if (!(timestamp < priv->iduration && priv->amr_size > 0 && priv->start_pos > 0)) {
        LOGE(TAG, "seek failed. ts = %llu, idu = %llu, amr_size = %d, spos = %u",
             timestamp, priv->iduration, priv->amr_size, priv->start_pos);
        return -1;
    }

    new_pos  = (uint64_t)((1.0 * timestamp / priv->iduration) * priv->amr_size) / packet_size * packet_size;
    new_pos += priv->start_pos;
    rc = stream_seek(o->s, new_pos, SEEK_SET);

    return rc;
}

static int _demux_amr_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_amr = {
    .name            = "amr",
    .type            = AVFORMAT_TYPE_AMR,
    .extensions      = { "amr", "anb", "awb", NULL },

    .read_probe      = _demux_amr_probe,

    .open            = _demux_amr_open,
    .close           = _demux_amr_close,
    .read_packet     = _demux_amr_read_packet,
    .seek            = _demux_amr_seek,
    .control         = _demux_amr_control,
};
#endif

