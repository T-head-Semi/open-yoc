/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_FLAC) && CONFIG_DEMUXER_FLAC
#include "avformat/avformat_utils.h"
#include "avformat/flac_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                                     "demux_flac"
#define FLAC_SYNC_HDR_MAX                       (FLAC_FRAME_SIZE_DEFAULT * 4)

typedef struct flac_sync_block {
    uint8_t                    *data;
    size_t                     cap;              ///< total capcity
    size_t                     size;             ///< length of avilable data
    size_t                     offset_r;         ///< read offset
    int32_t                    offset_sync1;     ///< flac sync 1 offset, -1 means not found
    int32_t                    offset_sync2;     ///< flac sync 2 offset, -1 means not found
} fsb_t;

struct flac_priv {
    struct flac_stream_info    si;
    fsb_t                      fsb;

    int32_t                    sync_hdr_max;
    int32_t                    start_pos;
    int32_t                    flac_size;        ///< not contain the ID3 size, etc
    size_t                     bsize_fixed;      ///< block size
    uint64_t                   iduration;        ///< inner duration, base time_base
    struct flac_hdr            hinfo;
};

static int _demux_flac_probe(const avprobe_data_t *pd)
{
    if (strncmp((const char*)pd->buf, "fLaC", 4))
        return 0;

    return AVPROBE_SCORE_MAX;
}

static int _demux_flac_open(demux_cls_t *o)
{
    int rc;
    sf_t sf;
    int fsize;
    uint32_t block_size;
    uint8_t type, last = 0, si_find = 0;
    uint8_t hdr[4] = {0};
    struct flac_stream_info *si;
    uint8_t *extradata     = NULL;
    struct flac_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct flac_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    stream_read(o->s, hdr, 4);
    if (strncmp((const char*)hdr, "fLaC", 4)) {
        goto err;
    }

    si = &priv->si;
    while(!stream_is_eof(o->s) && !last) {
        rc = stream_read(o->s, hdr, 4);
        CHECK_RET_TAG_WITH_GOTO(rc == 4, err);
        last       = (hdr[0] >> 7) & 0x1;
        type       = hdr[0] & 0x7;
        block_size = byte_r24be((const uint8_t*)&hdr[1]);
        switch (type) {
        case FLAC_METADATA_TYPE_STREAMINFO: {
            if (block_size != FLAC_STREAMINFO_SIZE) {
                LOGE(TAG, "flac streaminfo size may be wrong");
                goto err;
            }
            extradata = av_zalloc(FLAC_STREAMINFO_SIZE);
            CHECK_RET_TAG_WITH_GOTO(extradata, err);
            rc = stream_read(o->s, extradata, FLAC_STREAMINFO_SIZE);
            CHECK_RET_TAG_WITH_GOTO(rc == FLAC_STREAMINFO_SIZE, err);

            rc = flac_parse_si((const uint8_t*)extradata, si);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
            si->fsize_min  = si->fsize_min ? si->fsize_min : FLAC_FRAME_SIZE_DEFAULT;
            si->fsize_max  = si->fsize_max ? si->fsize_max : FLAC_FRAME_SIZE_DEFAULT;
            si_find        = 1;
            break;
        }
        default:
            stream_skip(o->s, block_size);
            break;
        }
    }

    if (!si_find) {
        LOGE(TAG, "stream info not found");
        goto err;
    }
    priv->sync_hdr_max = si->fsize_max >= FLAC_SYNC_HDR_MAX ? si->fsize_max + 128 : FLAC_SYNC_HDR_MAX;
    priv->fsb.data     = av_malloc(priv->sync_hdr_max);
    CHECK_RET_TAG_WITH_GOTO(priv->fsb.data, err);
    priv->fsb.cap          = priv->sync_hdr_max;
    priv->fsb.offset_sync1 = -1;
    priv->fsb.offset_sync2 = -1;
    priv->bsize_fixed      = si->bsize_min == si->bsize_max ? si->bsize_min : 0;
    priv->start_pos        = stream_tell(o->s);

    sf                     = sf_make_channel(si->channels) | sf_make_rate(si->rate) | sf_make_bit(si->bits) | sf_make_signed(1);
    fsize                  = stream_get_size(o->s);
    o->ash.sf              = sf;
    o->ash.id              = AVCODEC_ID_FLAC;
    o->ash.extradata       = extradata;
    o->ash.extradata_size  = extradata ? FLAC_STREAMINFO_SIZE : 0;
    o->time_scale          = AV_FLICK_BASE / si->rate;
    o->priv                = priv;
    if (si->nb_samples) {
        o->duration     = si->nb_samples * 1000 / si->rate;
        priv->iduration = o->duration / 1000.0 * o->time_scale;
        priv->flac_size = fsize > 0 ? fsize - priv->start_pos : 0;
    }

    return 0;
err:
    if (priv) {
        av_free(priv);
        av_free(priv->fsb.data);
    }
    av_free(extradata);
    return -1;
}

static int _demux_flac_close(demux_cls_t *o)
{
    struct flac_priv *priv = o->priv;

    av_free(priv->fsb.data);
    av_free(priv);
    o->priv = NULL;
    return 0;
}

/**
 * @brief  flac_sync read bytes from stream
 * @param  [in] opaque
 * @param  [in] bytes
 * @param  [in] size
 * @return -1 on error
 */
static int _sync_read_stream(void *opaque, uint8_t *bytes, size_t size)
{
    int rc = 0;
    fsb_t *fsb = opaque;

    if (fsb->offset_r < fsb->size) {
        rc = fsb->size - fsb->offset_r;
        rc = rc >= size ? size : rc;
        memcpy(bytes, fsb->data + fsb->offset_r, rc);
        fsb->offset_r += rc;
    }

    return rc;
}

//FIXME:
static int _demux_flac_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = -1;
    sf_t sf = o->ash.sf;
    int resync_cnt = 10, cnt = 0;
    int64_t sample_numbers = 0;
    int start_pos = -1, fsize = 0, remain;
    uint8_t hdr[FLAC_HDR_SIZE_MAX] = { 0 };
    struct flac_priv *priv         = o->priv;
    fsb_t *fsb                     = &priv->fsb;
    struct flac_stream_info *si    = &priv->si;
    struct flac_hdr info, *hinfo   = &info;

    memset(hinfo, 0, sizeof(struct flac_hdr));
    rc = stream_read(o->s, fsb->data + fsb->size, fsb->cap - fsb->size);
    if (rc <= 0) {
        if (fsb->size <= FLAC_HDR_SIZE_MIN) {
            goto err;
        }
    } else
        fsb->size += rc;

resync:
    rc = flac_sync(_sync_read_stream, fsb, priv->sync_hdr_max, hdr, hinfo);
    if (rc < 0) {
        /* may be the last packet */
        if (fsb->offset_sync1 >= 0 && (fsb->size - fsb->offset_sync1 > 0) && stream_is_eof(o->s)) {
            start_pos = fsb->offset_sync1;
            fsize     = fsb->size - fsb->offset_sync1;
        } else {
            LOGE(TAG, "flac sync failed, rc = %d", rc);
            goto err;
        }
    } else {
        if (!(hinfo->channels == sf_get_channel(sf) &&
              (hinfo->block_size > 0 && hinfo->block_size <= si->bsize_max) &&
              (hinfo->bits == sf_get_bit(sf)) &&
              (hinfo->rate == sf_get_rate(sf)))) {
            if (cnt > resync_cnt) {
                LOGE(TAG, "flac sync failed, rc = %d, resync_cnt = %d", rc, resync_cnt);
                goto err;
            }
            cnt++;
            hdr[0] = 0; /* reset hdr for resync */
            goto resync;
        }

        if (fsb->offset_sync1 < 0) {
            fsb->offset_sync1 = fsb->offset_r - FLAC_HDR_SIZE_MAX;
            fsb->offset_r     = fsb->offset_sync1 + FLAC_HDR_SIZE_MIN;
            memset(hdr, 0, FLAC_HDR_SIZE_MAX);
            /* get sync2 */
            goto resync;
        } else if (fsb->offset_sync2 < 0) {
            fsb->offset_sync2 = fsb->offset_r - FLAC_HDR_SIZE_MAX;
            start_pos = fsb->offset_sync1;
            fsize     = fsb->offset_sync2 - fsb->offset_sync1;
        }
    }

    if (hinfo->is_var)
        sample_numbers = hinfo->nb_frame_or_sample;
    else
        sample_numbers = priv->bsize_fixed ? hinfo->nb_frame_or_sample * priv->bsize_fixed : 0;

    if (pkt->size < fsize) {
        rc = avpacket_grow(pkt, fsize);
        if (rc < 0) {
            LOGE(TAG, "avpacket resize fail. pkt->size = %d, framesize = %d", pkt->size, fsize);
            goto err;
        }
    }

    memcpy(pkt->data, fsb->data + start_pos, fsize);
    pkt->len = fsize;

    remain = fsb->size - start_pos - fsize;
    memmove(fsb->data, fsb->data + start_pos + fsize, remain);
    fsb->size         = remain;
    fsb->offset_r     = FLAC_HDR_SIZE_MIN;
    fsb->offset_sync1 = 0;
    fsb->offset_sync2 = -1;
    if (sample_numbers > 0 && priv->flac_size > 0 && si->nb_samples > 0) {
        pkt->pts = 1.0 * sample_numbers / si->nb_samples * priv->iduration;
    }

    return fsize;
err:
    return rc;
}

static int _demux_flac_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    size_t new_pos;
    struct flac_priv *priv = o->priv;
    fsb_t *fsb             = &priv->fsb;

    if (!(timestamp < priv->iduration && priv->flac_size > 0 && priv->start_pos >= 0)) {
        LOGE(TAG, "seek failed. ts = %llu, idu = %llu, flac_size = %d, spos = %d",
             timestamp, priv->iduration, priv->flac_size, priv->start_pos);
        return -1;
    }

    fsb->size         = 0;
    fsb->offset_r     = 0;
    fsb->offset_sync1 = -1;
    fsb->offset_sync2 = -1;
    new_pos = priv->start_pos + (1.0 * timestamp / priv->iduration) * priv->flac_size;
    rc = stream_seek(o->s, new_pos, SEEK_SET);

    return rc;
}

static int _demux_flac_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_flac = {
    .name            = "flac",
    .type            = AVFORMAT_TYPE_FLAC,
    .extensions      = { "flac", NULL },

    .read_probe      = _demux_flac_probe,

    .open            = _demux_flac_open,
    .close           = _demux_flac_close,
    .read_packet     = _demux_flac_read_packet,
    .seek            = _demux_flac_seek,
    .control         = _demux_flac_control,
};
#endif

