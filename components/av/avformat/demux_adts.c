/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_ADTS) && CONFIG_DEMUXER_ADTS
#include "avformat/avformat_utils.h"
#include "avformat/adts_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                    "demux_adts"
#define ADTS_SYNC_HDR_MAX      (2*1024)

struct adts_priv {
    struct adts_hdr            hinfo;
    int32_t                    start_pos;
    uint64_t                   iduration;  ///< inner duration, base time_base
};

static int _demux_adts_probe(const avprobe_data_t *pd)
{
    int rc;
    int score = 0, max;
    bio_t bio;
    uint8_t hdr[ADTS_HDR_SIZE] = {0};
    struct adts_hdr hinfo = {0};

    bio_init(&bio, pd->buf, pd->buf_size);
    bio_read(&bio, hdr, ADTS_HDR_SIZE);
    rc = adts_sync(sync_read_bio, &bio, ADTS_SYNC_HDR_MAX, hdr, &hinfo);
    if (rc >= 0) {
        max = rc == 0 ? AVPROBE_SCORE_MAX : AVPROBE_SCORE_AGAIN;
        /* judge twice */
        memset(hdr, 0, sizeof(hdr));
        bio_skip(&bio, hinfo.flen - ADTS_HDR_SIZE);
        bio_read(&bio, hdr, ADTS_HDR_SIZE);
        rc = adts_hdr_get(hdr, &hinfo);
        score = rc == 0 ? max : 0;
        LOGD(TAG, "adts probe result: sync_cnt = %d, score = %d", rc, score);
    }

    return score;
}

static int _demux_adts_open(demux_cls_t *o)
{
    int rc;
    int fsize;
    int bits = 16, flen;
    uint8_t hdr[ADTS_HDR_SIZE] = {0};
    uint8_t *extradata = NULL;
    struct adts_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct adts_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    rc = adts_sync(sync_read_stream, o->s, ADTS_SYNC_HDR_MAX, hdr, &priv->hinfo);
    if (rc < 0) {
        LOGE(TAG, "adts sync failed, rc = %d", rc);
        goto err;
    } else {
        flen      = priv->hinfo.flen;
        extradata = av_zalloc(flen);
        CHECK_RET_TAG_WITH_GOTO(extradata, err);
        priv->start_pos = stream_tell(o->s) - ADTS_HDR_SIZE;
        memcpy(extradata, hdr, ADTS_HDR_SIZE);
        stream_read(o->s, extradata + ADTS_HDR_SIZE, flen - ADTS_HDR_SIZE);
        stream_seek(o->s, priv->start_pos, SEEK_SET);
    }

    fsize                 = stream_get_size(o->s);
    o->priv               = priv;
    o->ash.id             = AVCODEC_ID_AAC;
    o->ash.sf             = sf_make_channel(priv->hinfo.channel) | sf_make_rate(priv->hinfo.freq) | sf_make_bit(bits) | sf_make_signed(bits > 8);
    /* frame header as extrdata */
    o->ash.extradata      = extradata;
    o->ash.extradata_size = flen;
    o->time_scale         = AV_FLICK_BASE / priv->hinfo.freq;
    /* may be not accurate, guess the duration */
    o->duration           = fsize > 0 ? fsize * 8 / priv->hinfo.bps * 1000.0 : 0;
    priv->iduration       = o->duration / 1000.0 * o->time_scale;

    return 0;
err:
    av_free(priv);
    av_free(extradata);
    return -1;
}

static int _demux_adts_close(demux_cls_t *o)
{
    struct adts_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _demux_adts_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = -1;
    int len = 0, offset, eof;
    int resync_cnt = 10, cnt = 0;
    uint8_t hdr[ADTS_HDR_SIZE] = {0};
    struct adts_hdr hinfo  = {0};
    struct adts_priv *priv = o->priv;

resync:
    rc = adts_sync(sync_read_stream, o->s, ADTS_SYNC_HDR_MAX, hdr, &hinfo);
    if (rc < 0) {
        LOGE(TAG, "adts sync failed, rc = %d", rc);
        goto err;
    } else {
        if (!(hinfo.id == priv->hinfo.id && hinfo.channel == priv->hinfo.channel && hinfo.freq == priv->hinfo.freq)) {
            if (cnt > resync_cnt) {
                LOGE(TAG, "adts sync failed, rc = %d, resync_cnt = %d", rc, resync_cnt);
                goto err;
            }
            cnt++;
            hdr[0] = 0; /* reset hdr for resync */
            goto resync;
        }
    }

    if (pkt->size < hinfo.flen) {
        rc = avpacket_grow(pkt, hinfo.flen);
        if (rc < 0) {
            LOGE(TAG, "avpacket resize fail. pkt->size = %d, flen = %d", pkt->size, hinfo.flen);
            goto err;
        }
    }

    offset = ADTS_HDR_SIZE;
    memcpy(pkt->data, hdr, offset);

    len = stream_read(o->s, pkt->data + offset, hinfo.flen - offset);
    if (len != hinfo.flen - offset) {
        LOGE(TAG, "stream read err, len = %d, diff = %d, frame size = %d,  offset = %d", len, hinfo.flen - offset, hinfo.flen, offset);
        goto err;
    }

    len      += offset;
    pkt->len  = len;
    if (priv->iduration > 0 && priv->start_pos >= 0) {
        pkt->pts = 1.0 * (stream_tell(o->s) - priv->start_pos) / (stream_get_size(o->s) - priv->start_pos) * priv->iduration;
    }

    return len;
err:
    eof = stream_is_eof(o->s);
    rc = eof ? 0 : rc;
    LOGI(TAG, "read packet may be eof. eof = %d, rc = %d, url = %s", eof, rc, stream_get_url(o->s));
    return rc;
}

static int _demux_adts_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    size_t new_pos;
    struct adts_priv *priv = o->priv;

    if (!(timestamp < priv->iduration && stream_get_size(o->s) > 0 && priv->start_pos >= 0)) {
        LOGE(TAG, "seek failed. ts = %llu, idu = %llu, file_size = %d, spos = %d",
             timestamp, priv->iduration, stream_get_size(o->s), priv->start_pos);
        return -1;
    }

    new_pos = priv->start_pos + (1.0 * timestamp / priv->iduration) * (stream_get_size(o->s) - priv->start_pos);
    rc = stream_seek(o->s, new_pos, SEEK_SET);

    return rc;
}

static int _demux_adts_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_adts = {
    .name            = "adts",
    .type            = AVFORMAT_TYPE_ADTS,
    .extensions      = { "adts", "aac", NULL },

    .read_probe      = _demux_adts_probe,

    .open            = _demux_adts_open,
    .close           = _demux_adts_close,
    .read_packet     = _demux_adts_read_packet,
    .seek            = _demux_adts_seek,
    .control         = _demux_adts_control,
};
#endif

