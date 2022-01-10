/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_MP3) && CONFIG_DEMUXER_MP3
#include "avformat/avformat_utils.h"
#include "avformat/mp3_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                    "demux_mp3"
#define MP3_SYNC_HDR_MAX       (2*1024)

static const uint8_t mp3_side_tbl[2][2] = {
    {32, 17},
    {17, 9}
};

static int _demux_mp3_read_packet(demux_cls_t *o, avpacket_t *pkt);

struct mp3_priv {
    uint8_t                    is_cbr;
    size_t                     nb_frames;  ///< numbers of mp3 frame
    int32_t                    start_pos;
    int32_t                    mp3_size;   ///< not contain the ID3 size, etc
    uint64_t                   iduration;  ///< inner duration, base time_base
    struct mp3_hdr_info        hinfo;
};

static int _demux_mp3_probe(const avprobe_data_t *pd)
{
    int score = 0;
    int rc, sync_cnt;
    bio_t bio;
    uint8_t hdr[MP3_HDR_LEN];
    struct mp3_hdr_info hinfo;

    bio_init(&bio, pd->buf, pd->buf_size);
    bio_read(&bio, hdr, MP3_HDR_LEN);
    sync_cnt = mp3_sync(sync_read_bio, &bio, MP3_SYNC_HDR_MAX, hdr, &hinfo);
    if (sync_cnt >= 0) {
        /* judge twice */
        memset(hdr, 0, sizeof(hdr));
        bio_skip(&bio, hinfo.framesize - MP3_HDR_LEN);
        bio_read(&bio, hdr, MP3_HDR_LEN);
        rc = mp3_hdr_get(hdr, &hinfo);
        if (sync_cnt)
            score = rc ? 0 : AVPROBE_SCORE_AGAIN;
        else
            score = rc ? 0 : AVPROBE_SCORE_MAX;

        LOGD(TAG, "mp3 probe, sync_cnt = %d, rc = %d, score = %d", sync_cnt, rc, score);
    }

    return score;
}

static int _demux_mp3_open(demux_cls_t *o)
{
    int rc;
    sf_t sf;
    bio_t bio;
    int fsize;
    uint32_t val, rate;
    struct mp3_priv *priv;
    struct mp3_hdr_info *hinfo;

    priv = av_zalloc(sizeof(struct mp3_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    o->priv = priv;

    rc = _demux_mp3_read_packet(o, &o->fpkt);
    CHECK_RET_TAG_WITH_GOTO(rc > 0, err);

    hinfo = &priv->hinfo;
    sf    = hinfo->sf;
    rate  = sf_get_rate(sf);
    bio_init(&bio, o->fpkt.data, o->fpkt.len);
    /* skip the side info */
    bio_skip(&bio, MP3_HDR_LEN + mp3_side_tbl[hinfo->lsf == 1][sf_get_channel(sf) == 1]);
    val = bio_r32le(&bio);
    priv->is_cbr = val == TAG_VAL('I', 'n', 'f', 'o');
    if (priv->is_cbr || val == TAG_VAL('X', 'i', 'n', 'g')) {
        val = bio_r32be(&bio);
        if (val & 0x1)
            priv->nb_frames = bio_r32be(&bio);
        if (val & 0x2)
            priv->mp3_size = bio_r32be(&bio);
    }

    fsize         = stream_get_size(o->s);
    o->ash.sf     = sf;
    o->ash.id     = AVCODEC_ID_MP3;
    o->time_scale = AV_FLICK_BASE / rate;
    o->fpkt.pts   = hinfo->spf * o->time_scale / rate;
    if (fsize > 0) {
        o->duration     = priv->nb_frames > 0 ? priv->nb_frames * hinfo->spf / (rate / 1000.0) : fsize * 8 / hinfo->bitrate;
        priv->iduration = o->duration / 1000.0 * o->time_scale;
        priv->start_pos = stream_tell(o->s) - o->fpkt.len;
        priv->mp3_size  = priv->mp3_size > 0 ? priv->mp3_size : fsize - priv->start_pos;
    }

    return 0;
err:
    av_free(priv);
    o->priv = NULL;
    return -1;
}

static int _demux_mp3_close(demux_cls_t *o)
{
    struct mp3_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _demux_mp3_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = -1;
    int64_t pos;
    int len = 0, mp3_guess;
    int resync_cnt = 10, cnt = 0;
    uint8_t hdr[MP3_HDR_LEN];
    struct mp3_priv *priv = o->priv;
    sf_t sf = priv->hinfo.sf;
    struct mp3_hdr_info info, *hinfo = &priv->hinfo;

    len = stream_read(o->s, hdr, MP3_HDR_LEN);
    if (len != MP3_HDR_LEN) {
        goto err;
    }
    //FIXME: patch for id3v1
    if (!strncmp((const char*)hdr, "TAG", 3)) {
        return 0;
    }
resync:
    mp3_guess = 0;
    pos = stream_tell(o->s);
    memset(&info, 0, sizeof(struct mp3_hdr_info));
    rc = mp3_sync(sync_read_stream, o->s, MP3_SYNC_HDR_MAX, hdr, &info);
    if (rc < 0) {
        LOGE(TAG, "mp3 sync failed, rc = %d", rc);
        goto err;
    }
    if (sf) {
        //FIXME: patch for seek
        if (!(hinfo->sf == info.sf && hinfo->layer == info.layer && hinfo->spf == info.spf)) {
            if (cnt > resync_cnt) {
                LOGE(TAG, "mp3 sync failed, rc = %d, resync_cnt = %d", rc, resync_cnt);
                goto err;
            }
            cnt++;
            hdr[0] = 0; /* reset hdr for resync */
            goto resync;
        }
    }
    if (rc > 0) {
        //FIXME:
        if (stream_is_seekable(o->s)) {
            /* guess may be wrong, judge twice needed*/
            mp3_guess = rc;
        }
    }

    if (pkt->size < info.framesize) {
        rc = avpacket_grow(pkt, info.framesize);
        if (rc < 0) {
            LOGE(TAG, "avpacket resize fail. pkt->size = %d, framesize = %d", pkt->size, info.framesize);
            goto err;
        }
    }

    memcpy(pkt->data, hdr, MP3_HDR_LEN);
    len = stream_read(o->s, pkt->data + MP3_HDR_LEN, info.framesize - MP3_HDR_LEN);
    if (len != info.framesize - MP3_HDR_LEN) {
        LOGE(TAG, "stream read err, len = %d, diff = %d, frame size = %d", len, info.framesize - MP3_HDR_LEN, info.framesize);
        goto err;
    }
    len += MP3_HDR_LEN;
    pkt->len = len;
    if (priv->iduration > 0 && priv->mp3_size > 0 && priv->start_pos >= 0) {
        pkt->pts = 1.0 * (stream_tell(o->s) - priv->start_pos) / priv->mp3_size * priv->iduration;
    }

    if (mp3_guess) {
        int elen, again_err = 0, new_pos;
        struct mp3_hdr_info einfo = {0};

        elen = stream_read(o->s, hdr, MP3_HDR_LEN);
        if (elen != MP3_HDR_LEN) {
            rc = -1;
            goto err;
        }
        rc = mp3_hdr_get(hdr, &einfo);
        if (rc == 0) {
            if (sf) {
                if (!(hinfo->sf == einfo.sf && hinfo->layer == einfo.layer && hinfo->spf == einfo.spf)) {
                    again_err = 1;
                }
            }
        } else {
            again_err = 1;
        }

        if (again_err) {
            new_pos = pos + mp3_guess + 1;
            rc = stream_seek(o->s, new_pos, SEEK_SET);
            if (rc < 0) {
                LOGE(TAG, "stream seek err, seek pos = %lld", new_pos);
                goto err;
            } else {
                LOGD(TAG, "guess again, retry");
                pkt->len = 0;
                goto resync;
            }
        } else {
            /* guess again ok, seek back */
            rc = stream_seek(o->s, -MP3_HDR_LEN, SEEK_CUR);
            CHECK_RET_TAG_WITH_GOTO(rc >= 0, err);
        }
    }
    //printf("=====>>len = %10d, pos = %lld\n", len, stream_tell(o->s) - len);
    if (sf == 0)
        memcpy(hinfo, &info, sizeof(info));

    return len;
err:
    return rc;
}

static int _demux_mp3_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    size_t new_pos;
    struct mp3_priv *priv      = o->priv;

    if (!(timestamp < priv->iduration && priv->mp3_size > 0 && priv->start_pos >= 0)) {
        LOGE(TAG, "seek failed. ts = %llu, idu = %llu, mp3_size = %d, spos = %d",
             timestamp, priv->iduration, priv->mp3_size, priv->start_pos);
        return -1;
    }

    new_pos = priv->start_pos + (1.0 * timestamp / priv->iduration) * priv->mp3_size;
    rc = stream_seek(o->s, new_pos, SEEK_SET);

    return rc;
}

static int _demux_mp3_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_mp3 = {
    .name            = "mp3",
    .type            = AVFORMAT_TYPE_MP3,
    .extensions      = { "mp3", "mp2", NULL },

    .read_probe      = _demux_mp3_probe,

    .open            = _demux_mp3_open,
    .close           = _demux_mp3_close,
    .read_packet     = _demux_mp3_read_packet,
    .seek            = _demux_mp3_seek,
    .control         = _demux_mp3_control,
};
#endif

