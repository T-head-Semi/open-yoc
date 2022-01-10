/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_TS) && CONFIG_DEMUXER_TS
#include "avutil/common.h"
#include "avutil/misc.h"
#include "avutil/mem_block.h"
#include "avformat/avformat_utils.h"
#include "avformat/adts_rw.h"
#include "avformat/mp3_rw.h"
#include "avformat/ts_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                   "demux_ts"

#define TS_SYNC_CNT           (3)
#define TS_SYNC_HDR_MAX       (2*1024)

static int _demux_ts_read_packet(demux_cls_t *o, avpacket_t *pkt);

enum tsf_type {
    TSF_TYPE_UNKOWN,
    TSF_TYPE_PES,
    TSF_TYPE_SECTION,
};

typedef struct tsf {
    enum tsf_type              type;
    int                        pid;
    int                        last_cc;
    slist_t                    node;
} tsf_t;

typedef int (*section_cb_t)    (tsf_t *f, const uint8_t *buf, int len);
typedef int (*pes_cb_t)        (tsf_t *f, const uint8_t *buf, int len, int is_start);

struct tsf_pes {
    tsf_t                      base;

    mblock_t                   *mb;
    pes_cb_t                   cb;
    void                       *arg;
};

struct tsf_section {
    tsf_t                      base;

    uint8_t                    *buf;                    ///< section buf
    int                        size;                    ///< total size of push data
    int                        len;                     ///< section length
    uint8_t                    sec_end;
    section_cb_t               cb;
    void                       *arg;
};

struct ts_priv {
    uint8_t                   stop_parse;               ///<
    uint8_t                   audio_es_type;            ///<
    uint16_t                  audio_pid;                ///< pid for audio player

    slist_t                   tsf_list;
    avpacket_t                *pkt;                     ///< point to the caller
};

static int _push_data_to_pes(tsf_t *f, const uint8_t *buf, int size, int is_start)
{
    int rc;
    uint8_t *data;
    size_t pl_start, pl_size;
    int pes_size, pes_hdr_size;
    struct tsf_pes *pes  = (struct tsf_pes*)f;
    struct ts_priv *priv = pes->arg;
    mblock_t *mb         = pes->mb;
    avpacket_t *pkt      = priv->pkt;

    if (is_start) {
        //reset the pes buf
        mb->used = 0;
    }

    rc = mblock_grow(mb, mb->used + size);
    CHECK_RET_TAG_WITH_RET(rc == 0, -1);
    data = mb->data;
    memcpy(data + mb->used, buf, size);
    mb->used += size;

    if (mb->used > PES_HEADER_SIZE) {
        if (byte_r24le(data) != 0x10000) {
            return -1;
        }
        pes_size = byte_r16be(data + 4);
        pes_hdr_size = data[8];
        if (pes_size > pes_hdr_size && (pes_size + 6) <= mb->used) {
            priv->stop_parse = 1;
            pl_start         = PES_HEADER_SIZE + pes_hdr_size;
            pl_size          = pes_size - 3 - pes_hdr_size;
            rc = avpacket_grow(pkt, pl_size);
            if (rc < 0) {
                LOGE(TAG, "avpacket resize fail. pkt->size = %d, size = %d", pkt->size, pl_size);
                return -1;
            }
            memcpy(pkt->data, data + pl_start, pl_size);
            pkt->len = pl_size;
            mb->used = 0;
        }
    }

    return 0;
}

static int _push_data_to_section(tsf_t *f, const uint8_t *buf, size_t size, int is_start)
{
    int rc;
    int remain, len;
    struct tsf_section *sec = (struct tsf_section*)f;

    remain = is_start ? SECTION_SIZE_MAX : SECTION_SIZE_MAX - sec->size;
    CHECK_RET_TAG_WITH_RET(remain >= size, -1);

    if (is_start) {
        memcpy(sec->buf, buf, size);
        sec->size = size;
        sec->len  = 0;
    } else {
        memcpy(sec->buf + sec->size, buf, size);
        sec->size += size;
    }

    len = (((sec->buf[1] & 0x0F) << 8) | sec->buf[2]) + 3;
    if (sec->size >= 3 && sec->size >= len) {
        rc = av_crc32(sec->buf, len);
        if (!rc) {
            sec->len = len;
            sec->cb(f, sec->buf, sec->len);
        }
    }

    return 0;
}

static tsf_t* _get_tsf(struct ts_priv *priv, uint16_t pid)
{
    tsf_t *f;

    slist_for_each_entry(&priv->tsf_list, f, tsf_t, node) {
        if (f->pid == pid) {
            return f;
        }
    }

    return NULL;
}

static tsf_t* _tsf_new(struct ts_priv *priv, uint16_t pid, enum tsf_type type, size_t fsize)
{
    tsf_t *f;

    f = _get_tsf(priv, pid);
    if (pid >= NB_PID_MAX || f || (fsize == 0)) {
        LOGE(TAG, "param err, pid = %u, f = %p, fsize = %u", pid, f, fsize);
        return NULL;
    }

    f = av_zalloc(fsize);
    CHECK_RET_TAG_WITH_RET(f, NULL);

    f->type    = type;
    f->pid     = pid;
    f->last_cc = -1;
    slist_add_tail(&f->node, &priv->tsf_list);

    return f;
}

static void _tsf_free(struct ts_priv *priv, tsf_t *f)
{
    if (f->type == TSF_TYPE_PES) {
        struct tsf_pes *pes = (struct tsf_pes*)f;

        mblock_free(pes->mb);
    } else if (f->type == TSF_TYPE_SECTION) {
        struct tsf_section *sec = (struct tsf_section*)f;

        av_free(sec->buf);
    }

    slist_del(&f->node, &priv->tsf_list);
    av_free(f);
}

static void _free_all_tsf(struct ts_priv *priv)
{
    tsf_t *f;
    slist_t *tmp;

    slist_for_each_entry_safe(&priv->tsf_list, tmp, f, tsf_t, node) {
        _tsf_free(priv, f);
    }
}

static tsf_t* _section_filter_new(struct ts_priv *priv, uint16_t pid, section_cb_t cb, void *arg)
{
    tsf_t *f;
    uint8_t *buf;
    struct tsf_section *sec;

    f = _tsf_new(priv, pid, TSF_TYPE_SECTION, sizeof(struct tsf_section));
    CHECK_RET_TAG_WITH_RET(f, NULL);

    buf = av_zalloc(SECTION_SIZE_MAX);
    CHECK_RET_TAG_WITH_GOTO(buf, err);
    sec           = (struct tsf_section*)f;
    sec->cb       = cb;
    sec->arg      = arg;
    sec->buf      = buf;

    return f;
err:
    _tsf_free(priv, f);
    return NULL;
}

static tsf_t* _pes_filter_new(struct ts_priv *priv, uint16_t pid, pes_cb_t cb, void *arg)
{
    tsf_t *f;
    mblock_t *mb = NULL;
    struct tsf_pes *pes;

    f = _tsf_new(priv, pid, TSF_TYPE_PES, sizeof(struct tsf_pes));
    CHECK_RET_TAG_WITH_RET(f, NULL);

    mb = mblock_new(1024, 0);
    CHECK_RET_TAG_WITH_GOTO(mb, err);

    pes      = (struct tsf_pes*)f;
    pes->mb  = mb;
    pes->cb  = cb;
    pes->arg = arg;

    return f;
err:
    _tsf_free(priv, f);
    return NULL;
}

//FIXME: find one audio pid only
static int _pmt_cb(tsf_t *f, const uint8_t *section, int size)
{
    const uint8_t *p = section;
    struct tsf_section *sec = (struct tsf_section*)f;
    struct ts_priv *priv    = sec->arg;
    int tid, pid, info_len, pos, es_type;

    tid = p[0];
    CHECK_RET_TAG_WITH_RET(tid == PMT_TID, -1);
    info_len = ((p[10] << 8) | p[11]) & 0xfff;
    pos = info_len + 12;

    for (; pos < size - 4;) {
        es_type = p[pos];
        pid = ((p[pos + 1] << 8) | p[pos + 2]) & 0x1fff;
        info_len = ((p[pos + 3] << 8) | p[pos + 4]) & 0xfff;
        if (AVMEDIA_TYPE_AUDIO == get_media_type_by_es(es_type)) {
            priv->audio_pid     = pid;
            priv->audio_es_type = es_type;
            if (!_get_tsf(priv, pid)) {
                _pes_filter_new(priv, pid, _push_data_to_pes, priv);
            }
            break;
        }
        pos += info_len + 5;
    }

    return 0;
}

//FIXME
static int _pat_cb(tsf_t *f, const uint8_t *section, int size)
{
    int n, tid, pnum, pid;
    const uint8_t *p = section;
    struct tsf_section *sec = (struct tsf_section*)f;
    struct ts_priv *priv    = sec->arg;

    tid = p[0];
    CHECK_RET_TAG_WITH_RET(tid == PAT_TID, -1);
    for (n = 0; n < size - 12; n += 4) {
        pnum = (p[8 + n] << 8) | p[9 + n];
        if (pnum != 0) {
            pid = ((p[10 + n] << 8) | p[11 + n]) & 0x1fff;
            if (!_get_tsf(priv, pid)) {
                _section_filter_new(priv, pid, _pmt_cb, priv);
            }
        }
    }

    return 0;
}

static int _demux_ts_probe(const avprobe_data_t *pd)
{
    int i = 0, sync_cnt = 0;
    uint8_t *buf = pd->buf;
    int size = pd->buf_size, max;

    while (i < size && (!(buf[i] == 0x47 && buf[i + TS_PACKET_SIZE] == 0x47)))
        i++;

    while (i < size && buf[i] == 0x47 && sync_cnt < TS_SYNC_CNT) {
        i += TS_PACKET_SIZE;
        sync_cnt++;
    }
    max = buf[0] == 0x47 ? AVPROBE_SCORE_MAX : AVPROBE_SCORE_AGAIN;

    return sync_cnt == TS_SYNC_CNT ? max : 0;
}

static int _read_sync_packet(stream_cls_t *s, uint8_t packet[TS_PACKET_SIZE])
{
    int rc, i = 0;

    while ((!stream_is_eof(s)) && (i++ < TS_SYNC_HDR_MAX)) {
        packet[0] = stream_r8(s);
        if (packet[0] == 0x47) {
            rc = stream_read(s, &packet[1], TS_PACKET_SIZE - 1);
            if (rc == TS_PACKET_SIZE - 1) {
                return 0;
            }
        }
    }

    return -1;
}

static int _parse_one_packet(demux_cls_t *o, const uint8_t buf[TS_PACKET_SIZE])
{
    int rc = 0;
    tsf_t *f;
    const uint8_t *p_cur, *p_end;
    struct ts_priv *priv = o->priv;
    int pid, is_start, cc, next_cc, cc_ok, afc, has_adt, has_pl, is_discon;

    pid = ((buf[1] & 0x1F) << 8) | buf[2];
    if (priv->audio_pid && priv->audio_pid != pid) {
        return 0;
    }

    is_start = (buf[1] >> 6) & 0x01;
    f = _get_tsf(priv, pid);
    if (priv->audio_pid && !f && is_start) {
        //add pes stream
    }
    if (!f)
        return 0;

    cc = (buf[3] & 0xf);
    afc = (buf[3] >> 4) & 0x03;
    if (afc == 0)
        return 0;
    has_pl    = afc & 1;
    has_adt   = afc & 2;
    is_discon = has_adt && (buf[4] > 0) && (buf[5] & 0x80);

    next_cc   = has_pl ? (f->last_cc + 1) & 0x0f : f->last_cc;
    cc_ok     = (next_cc == cc) || is_discon || (f->last_cc < 0);
    if (!cc_ok) {
        //FIXME:
        LOGD(TAG, "not continue, cc = %d, next_cc = %d, pid = %d, is_start = %d", cc, next_cc, pid, is_start);
    }

    f->last_cc = cc;
    p_end = buf + TS_PACKET_SIZE;
    p_cur = buf + 4;
    p_cur = has_adt ? p_cur + buf[4] + 1 : p_cur;
    if (!(p_cur < p_end && has_pl))
        return 0;

    if (f->type == TSF_TYPE_PES) {
        rc = _push_data_to_pes(f, p_cur, p_end - p_cur, is_start);
    } else if (f->type == TSF_TYPE_SECTION) {
        p_cur = is_start ? p_cur + p_cur[0] + 1 : p_cur;
        rc = _push_data_to_section(f, p_cur, p_end - p_cur, is_start);
    }

    return rc;
}

static int _parse_packets(demux_cls_t *o, int nb_packets)
{
    int rc;
    int cnt = 0;
    uint8_t packet[TS_PACKET_SIZE];
    struct ts_priv *priv = o->priv;

    priv->stop_parse = 0;
    for (;;) {
        rc = _read_sync_packet(o->s, packet);
        if (rc < 0)
            break;

        rc = _parse_one_packet(o, packet);
        if (rc < 0)
            break;

        if (nb_packets) {
            if (priv->audio_pid || cnt++ > nb_packets)
                break;
        }
        if (priv->stop_parse)
            break;
    }

    return rc;
}

static int _demux_ts_open(demux_cls_t *o)
{
    int rc;
    tsf_t *f;
    uint8_t *extradata = NULL;
    int esize;
    struct ts_priv *priv;

    priv = av_zalloc(sizeof(struct ts_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    o->priv = priv;

    f = _section_filter_new(priv, PAT_TID, _pat_cb, priv);
    CHECK_RET_TAG_WITH_GOTO(f, err);

    rc = _parse_packets(o, TS_SYNC_HDR_MAX / TS_PACKET_SIZE);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = _demux_ts_read_packet(o, &o->fpkt);
    CHECK_RET_TAG_WITH_GOTO(rc > 0, err);

    o->ash.id = get_codecid_by_es(priv->audio_es_type);
    if (o->ash.id == AVCODEC_ID_AAC) {
        int bits = 16;
        struct adts_hdr hinfo = {0};

        rc = adts_hdr_get(o->fpkt.data, &hinfo);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        o->ash.sf = sf_make_channel(hinfo.channel) | sf_make_rate(hinfo.freq) | sf_make_bit(bits) | sf_make_signed(bits > 8);

        esize = o->fpkt.len;
        extradata = av_zalloc(esize);
        CHECK_RET_TAG_WITH_GOTO(extradata, err);
        memcpy(extradata, o->fpkt.data, esize);
        o->ash.extradata      = extradata;
        o->ash.extradata_size = esize;
    } else if (o->ash.id == AVCODEC_ID_MP3) {
        struct mp3_hdr_info hinfo;

        rc = mp3_hdr_get((const uint8_t*)o->fpkt.data, &hinfo);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        o->ash.sf = hinfo.sf;
    }

    return 0;
err:
    av_free(extradata);
    if (priv) {
        _free_all_tsf(priv);
        av_free(priv);
    }
    o->priv = NULL;
    return -1;
}

static int _demux_ts_close(demux_cls_t *o)
{
    struct ts_priv *priv = o->priv;

    _free_all_tsf(priv);
    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _demux_ts_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = -1;
    struct ts_priv *priv = o->priv;

    pkt->len = 0;
    priv->pkt = pkt;
    rc = _parse_packets(o, 0);

    return rc < 0 ? rc : pkt->len;
}

static int _demux_ts_seek(demux_cls_t *o, uint64_t timestamp)
{
    //TODO
    return -1;
}

static int _demux_ts_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_ts = {
    .name            = "ts",
    .type            = AVFORMAT_TYPE_TS,
    .extensions      = { "ts", NULL },

    .read_probe      = _demux_ts_probe,

    .open            = _demux_ts_open,
    .close           = _demux_ts_close,
    .read_packet     = _demux_ts_read_packet,
    .seek            = _demux_ts_seek,
    .control         = _demux_ts_control,
};
#endif

