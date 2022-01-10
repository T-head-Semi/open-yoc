/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_OGG) && CONFIG_DEMUXER_OGG
#include "avformat/avformat_utils.h"
#include "avformat/ogg_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                    "demux_ogg"
#define OGG_SYNC_HDR_MAX       (2*1024)

struct ogg_priv {
    uint8_t                    channels;
    uint8_t                    nb_segs;
    uint8_t                    seg_idx;
    uint8_t                    seg_table[OGG_NB_SEG_MAX];
    uint32_t                   serial_nb;                 ///< stream serial number for play
    int                        rate;
    int                        header;

    uint8_t                    *data;                     ///< segment data of page
    size_t                     len;                       ///< length of avilable data
    size_t                     pos;                       ///< read pos of data
    size_t                     size;                      ///< capcity of data
};

static int _demux_ogg_read_packet(demux_cls_t *o, avpacket_t *pkt);

static int _demux_ogg_probe(const avprobe_data_t *pd)
{
    return strncmp((const char*)pd->buf, "OggS", 4) == 0 ? AVPROBE_SCORE_MAX : 0;
}

static int _ogg_read_page(demux_cls_t *o, struct page_hdr *phdr)
{
    int rc = -1;
    int len = 0, i;
    uint8_t *data;
    uint8_t buf[OGG_HDR_LEN] = {0};
    struct ogg_priv *priv = o->priv;

    priv->pos     = 0;
    priv->len     = 0;
    priv->nb_segs = 0;
    priv->seg_idx = 0;
    memset(phdr, 0, sizeof(struct page_hdr));
    memset(priv->seg_table, 0, sizeof(priv->seg_table));
resync:
    rc = ogg_page_sync(sync_read_stream, o->s, OGG_SYNC_HDR_MAX, buf, phdr);
    if (rc < 0) {
        LOGE(TAG, "ogg sync failed, rc = %d", rc);
        goto err;
    }
    rc = stream_read(o->s, priv->seg_table, phdr->nb_segs);
    CHECK_RET_TAG_WITH_GOTO(rc == phdr->nb_segs, err);

    len = 0;
    for (i = 0; i < phdr->nb_segs; i++) {
        len += priv->seg_table[i];
    }
    if (len == 0) {
        /* patch: len may be zero sometimes */
        goto resync;
    }

    if (len > priv->size) {
        data = av_realloc(priv->data, len);
        CHECK_RET_TAG_WITH_GOTO(data, err);
        priv->data = data;
        priv->size = len;
    }

    rc = stream_read(o->s, priv->data, len);
    CHECK_RET_TAG_WITH_GOTO(rc == len, err);
    priv->len     = len;
    priv->nb_segs = phdr->nb_segs;

    return 0;
err:
    return -1;
}

static int _ogg_read_pkt(demux_cls_t *o, int *pstart, int *psize)
{
    int ret = -1, rc;
    struct ogg_priv *priv = o->priv;
    struct page_hdr hdr, *phdr = &hdr;
    int retry_cnt = 0, size = 0, seg_size, pkt_ok = 0;
    int page_read = 0, page_flag = priv->seg_idx == priv->nb_segs;

    do {
retry:
        if (priv->header || page_flag) {
            rc = _ogg_read_page(o, phdr);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
            page_read = 1;
        }

        if (o->ash.id == AVCODEC_ID_UNKNOWN) {
            o->ash.id = ogg_find_codec(priv->data, priv->len);
            if (o->ash.id == AVCODEC_ID_UNKNOWN) {
                if (retry_cnt++ < 3)
                    goto retry;
                else {
                    LOGE(TAG, "ogg find codec failed");
                    goto err;
                }
            }
            priv->serial_nb = phdr->serial_nb;
        }

        //FIXME: only read audio pkt
        if (page_read && priv->serial_nb != phdr->serial_nb)
            goto retry;

        if (priv->header) {
            if (o->ash.id == AVCODEC_ID_OPUS) {
                if (IS_FIRST_PAGE(phdr->type)) {
                    priv->rate     = 48000;
                    priv->channels = priv->data[9];
                } else {
                    if (priv->len < 8 || memcmp(priv->data, "OpusTags", 8))
                        goto err;
                    /* skip comment header */
                    priv->header = 0;
                }
            } else if (o->ash.id == AVCODEC_ID_SPEEX) {
                if (IS_FIRST_PAGE(phdr->type)) {
                    priv->rate     = byte_r32le(priv->data + 36);
                    priv->channels = byte_r32le(priv->data + 48);
                } else {
                    /* skip comment header */
                    priv->header = 0;
                }
            }
            page_flag = 1;
            goto retry;
        } else {
            while (priv->seg_idx < priv->nb_segs) {
                seg_size  = priv->seg_table[priv->seg_idx++];
                size     += seg_size;
                if (seg_size < 255) {
                    pkt_ok = 1;
                    break;
                }
            }
        }
    } while (!pkt_ok);

    ret        = 0;
    *psize     = size;
    *pstart    = priv->pos;
    priv->pos += size;
err:
    return ret;
}

static int _demux_ogg_open(demux_cls_t *o)
{
    int rc;
    struct ogg_priv *priv;

    priv = av_zalloc(sizeof(struct ogg_priv));
    priv->header = -1;
    o->priv      = priv;

    rc = _demux_ogg_read_packet(o, &o->fpkt);
    CHECK_RET_TAG_WITH_GOTO(rc > 0, err);
    o->ash.sf = sf_make_channel(priv->channels) | sf_make_rate(priv->rate) | sf_make_bit(16) | sf_make_signed(1);

    return 0;
err:
    if (priv) {
        av_free(priv->data);
        av_free(priv);
    }
    return -1;
}

static int _demux_ogg_close(demux_cls_t *o)
{
    struct ogg_priv *priv = o->priv;

    av_free(priv->data);
    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _demux_ogg_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int start, len;
    int ret = -1, rc;
    struct ogg_priv *priv = o->priv;

    rc = _ogg_read_pkt(o, &start, &len);
    if (rc == 0) {
        if (pkt->size < len) {
            rc = avpacket_grow(pkt, len);
            if (rc < 0) {
                LOGE(TAG, "avpacket resize fail. pkt->size = %d, len = %d", pkt->size, len);
                goto err;
            }
        }

        memcpy(pkt->data, priv->data + start, len);
        ret      = len;
        pkt->len = len;
    }

err:
    return ret;
}

static int _demux_ogg_seek(demux_cls_t *o, uint64_t timestamp)
{
    //TODO
    return -1;
}

static int _demux_ogg_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_ogg = {
    .name            = "ogg",
    .type            = AVFORMAT_TYPE_OGG,
    .extensions      = { "ogg", NULL },

    .read_probe      = _demux_ogg_probe,

    .open            = _demux_ogg_open,
    .close           = _demux_ogg_close,
    .read_packet     = _demux_ogg_read_packet,
    .seek            = _demux_ogg_seek,
    .control         = _demux_ogg_control,
};
#endif

