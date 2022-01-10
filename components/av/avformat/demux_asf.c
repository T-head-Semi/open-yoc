/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_ASF) && CONFIG_DEMUXER_ASF
#include "avformat/avformat_utils.h"
#include "avformat/riff_rw.h"
#include "avformat/asf_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                    "demux_asf"
#define ASF_SYNC_HDR_MAX       (2*1024)

#define GET_LEN_2BITS(bits) ((((bits) & 0x3) == 0x03) ? 4 : ((bits) & 0x3))
#define GET_VAL_2BITS(bits, var, defval)        \
    switch (bits & 3) {                         \
    case 3:                                     \
        var = stream_r32le(s);                  \
        break;                                  \
    case 2:                                     \
        var = stream_r16le(s);                  \
        break;                                  \
    case 1:                                     \
        var = stream_r8(s);                     \
        break;                                  \
    default:                                    \
        var = defval;                           \
        break;                                  \
    }

typedef struct {
    uint8_t                    ecc_flag;         ///< Error Correction Present
    uint8_t                    len_flag;
    uint8_t                    pro_flag;         ///< property flag
    uint8_t                    nb_segments;      ///< number of segments
    uint8_t                    seg_idx;          ///< cur index
    uint8_t                    pl_flag;          ///< payload flag

    int32_t                    oid;              ///<
    int32_t                    frag_offset;      ///<
    int32_t                    repl_size;        ///<
    int32_t                    frag_size;        ///<
    int32_t                    obj_size;        ///<

    int16_t                    duration;         ///< ms, pkt duration
    int32_t                    pos;              ///< postion
    int32_t                    ts;               ///< ms, pkt timestamp
    int32_t                    remain_size;      ///< remain size of asf pkt
    int32_t                    pad_size;         ///< pad size of asf pkt
} asf_packet_t;

struct asf_priv {
    size_t                     nb_packets;
    int64_t                    play_duration;    ///< 100 ns
    int64_t                    preroll;          ///< ms
    uint16_t                   sid;              ///< stream number(1~127)
    uint32_t                   bps;
    uint32_t                   block_align;
    size_t                     packet_size;

    uint8_t                    use_ecc;          ///< Error Correction Present

    int32_t                    start_pos;
    int32_t                    data_size;       ///< size of the data obj
    uint64_t                   iduration;       ///< inner duration, base time_base
    asf_packet_t               apkt;
};

static int _demux_asf_probe(const avprobe_data_t *pd)
{
    return ASF_GUID_HEADER == asf_guid_get_type((const uint8_t*)pd->buf) ? AVPROBE_SCORE_MAX : 0;
}

static int _asf_read_file_properties(demux_cls_t *o, int size)
{
    int rc = 0;
    stream_cls_t *s = o->s;
    int psize_min, psize_max;
    struct asf_priv *priv = o->priv;

    UNUSED(size);
    stream_skip(s, 16); /* FILEID */
    stream_skip(s, 8);  /* FILESIZE */
    stream_skip(s, 8);  /* CREATIONDATE */
    priv->nb_packets = stream_r64le(s);
    CHECK_RET_TAG_WITH_RET(priv->nb_packets > 0, -1);
    priv->play_duration = stream_r64le(s);
    stream_skip(s, 8);  /* SEND DURATION */
    priv->preroll = stream_r64le(s);
    CHECK_RET_TAG_WITH_RET(priv->preroll >= 0 && (priv->play_duration / 10000 - priv->preroll > 0), -1);
    stream_skip(s, 4);  /* flag */
    psize_min = stream_r32le(s);
    psize_max = stream_r32le(s);
    CHECK_RET_TAG_WITH_RET((psize_min == psize_max) && (psize_min > 0), -1);
    priv->packet_size = psize_min;

    return rc;
}

static int _asf_read_stream_properties(demux_cls_t *o, int size)
{
    int rc = 0;
    asf_guid_t type;
    int spec_size, cbsize;
    stream_cls_t *s = o->s;
    uint8_t guid[ASF_GUID_SIZE];
    struct asf_priv *priv = o->priv;
    unsigned int format_tag, channels, rate, bits;

    UNUSED(size);
    if (priv->sid) {
        LOGD(TAG, "find a audio stream already, ignore. sid = %u", priv->sid);
        return 0;
    }
    stream_read(s, guid, sizeof(guid));
    type = asf_guid_get_type((const uint8_t*)&guid);
    CHECK_RET_TAG_WITH_RET(type == ASF_GUID_STREAM_TYPE_AUDIO, 0);
    stream_skip(s, 16); /* ERROR CORRECTION TYPE */
    stream_skip(s, 8);  /* TIME OFFSET */
    spec_size = stream_r32le(s);
    stream_skip(s, 4);  /* LEN of ERROR CORRECTION TYPE */
    priv->sid = stream_r16le(s) & 0x7;
    CHECK_RET_TAG_WITH_RET(priv->sid, -1);
    stream_skip(s, 4);  /* RES */

    //FIXME:
    if (type == ASF_GUID_STREAM_TYPE_AUDIO) {
        format_tag        = stream_r16le(o->s);
        channels          = stream_r16le(o->s);
        rate              = stream_r32le(o->s);
        priv->bps         = stream_r32le(o->s) * 8;
        priv->block_align = stream_r16le(o->s);
        bits              = stream_r16le(o->s);
        if (spec_size >= 18) {
            cbsize = stream_r16le(o->s);
            cbsize = MIN(spec_size - 18, cbsize);
            if (cbsize > 0) {
                o->ash.extradata = av_malloc(cbsize);
                CHECK_RET_TAG_WITH_RET(o->ash.extradata, -1);
                stream_read(s, o->ash.extradata, cbsize);
                o->ash.extradata_size = cbsize;
            }
        }

        o->ash.id = wav_get_codec_id(format_tag);
        if (!(o->ash.id == AVCODEC_ID_WMAV1  || o->ash.id == AVCODEC_ID_WMAV2)) {
            LOGE(TAG, "unsupported format tag %u", format_tag);
            return -1;
        }
        o->ash.bps         = priv->bps;
        o->ash.block_align = priv->block_align;
        o->ash.sf          = sf_make_channel(channels) | sf_make_rate(rate) | sf_make_bit(bits) | sf_make_signed(bits > 8);
    }

    return rc;
}

static int _demux_asf_open(demux_cls_t *o)
{
    int rc;
    sf_t sf;
    asf_guid_t type;
    uint32_t rate;
    struct asf_priv *priv;
    stream_cls_t *s = o->s;
    uint8_t guid[ASF_GUID_SIZE];
    int fsize, hsize, size, nb_subobjs, pos;

    priv = av_zalloc(sizeof(struct asf_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    o->priv = priv;

    stream_read(s, guid, sizeof(guid));
    type = asf_guid_get_type((const uint8_t*)&guid);
    CHECK_RET_TAG_WITH_GOTO(type == ASF_GUID_HEADER, err);
    hsize = stream_r64le(s);
    CHECK_RET_TAG_WITH_GOTO(hsize > (16 + 8) * 4, err);

    nb_subobjs = stream_r32le(s);
    CHECK_RET_TAG_WITH_GOTO(nb_subobjs >= 3, err);
    stream_skip(s, 1); /* res1 */
    stream_skip(s, 1); /* res2 */

    while (!stream_is_eof(s)) {
        pos = stream_tell(o->s);
        stream_read(s, guid, sizeof(guid));
        type = asf_guid_get_type((const uint8_t*)&guid);
        size = stream_r64le(s);
        CHECK_RET_TAG_WITH_GOTO(size > (16 + 8), err);
        switch (type) {
        case ASF_GUID_FILE_PROPERTIES:
            rc = _asf_read_file_properties(o, size);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
            break;
        case ASF_GUID_STREAM_PROPERTIES:
            rc = _asf_read_stream_properties(o, size);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
            break;
        case ASF_GUID_DATA:
            stream_skip(s, 16); /* FILEID */
            stream_skip(s, 8);  /* TOTAL DATA PACKETS */
            stream_skip(s, 2);  /* res */
            priv->start_pos = pos + 50;
            priv->data_size = size - 50;
            break;
        default:
            break;
        }
        if (priv->start_pos) {
            /* start to read the packet */
            break;
        }
        stream_seek(o->s, pos + size, SEEK_SET);
    }

    sf   = o->ash.sf;
    rate = sf_get_rate(sf);
    if (!(sf && rate > 0 && priv->start_pos)) {
        LOGD(TAG, "find a audio stream already, ignore. sid = %u", priv->sid);
        goto err;
    }

    fsize         = stream_get_size(o->s);
    o->ash.sf     = sf;
    o->time_scale = AV_FLICK_BASE / rate;
    if (fsize > 0) {
        o->duration     = priv->play_duration / 10000 - priv->preroll;
        priv->iduration = o->duration / 1000.0 * o->time_scale;
    }

    return 0;
err:
    av_free(priv);
    o->priv = NULL;
    return -1;
}

static int _demux_asf_close(demux_cls_t *o)
{
    struct asf_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _asf_get_packet(demux_cls_t *o)
{
    uint8_t ecf;
    int rsize = 8, t1, t2;
    int pkt_len, seq_len, pad_len;
    stream_cls_t *s       = o->s;
    struct asf_priv *priv = o->priv;
    asf_packet_t *apkt    = &priv->apkt;

    apkt->pos = stream_tell(s);
    ecf = stream_r8(s);
    if (ecf == 0x82) {
        t1 = stream_r8(s);
        t2 = stream_r8(s);
        if (t1 || t2) {
            LOGE(TAG, "invalid ecc, t1 = %d, t2 = %d", t1, t2);
            goto err;
        }
        rsize += 3;
        priv->use_ecc  = 1;
        apkt->ecc_flag = ecf;
        apkt->len_flag = stream_r8(s);
        apkt->pro_flag = stream_r8(s);
    } else {
        if (priv->use_ecc) {
            LOGE(TAG, "ecc used before, may be not sync now.");
            goto err;
        }
        priv->use_ecc   = 0;
        apkt->len_flag = ecf;
        apkt->pro_flag = stream_r8(s);
    }

    GET_VAL_2BITS(apkt->len_flag >> 5, pkt_len, priv->packet_size);
    rsize += GET_LEN_2BITS(apkt->len_flag >> 5);
    GET_VAL_2BITS(apkt->len_flag >> 1, seq_len, 0);
    rsize += GET_LEN_2BITS(apkt->len_flag >> 1);
    GET_VAL_2BITS(apkt->len_flag >> 3, pad_len, 0);
    rsize += GET_LEN_2BITS(apkt->len_flag >> 3);

    if (pad_len > pkt_len || pkt_len <= 0 || pkt_len > priv->packet_size) {
        LOGE(TAG, "wrong pkt len, may be not sync, pad = %d, pkt = %d, seq = %d.", pad_len, pkt_len, seq_len);
        goto err;
    }
    apkt->ts       = stream_r32le(s);
    apkt->duration = stream_r16le(s);
    if (apkt->len_flag & 0x1) {
        apkt->pl_flag = stream_r8(s);
        rsize++;
        apkt->nb_segments = apkt->pl_flag & 0x3f;
        if (!(apkt->nb_segments && (apkt->pl_flag & 0x80))) {
            LOGE(TAG, "wrong payload flag, flag = %d.", apkt->pl_flag);
            goto err;
        }
    } else {
        apkt->pl_flag     = 0x80;
        apkt->nb_segments = 1;
    }
    apkt->remain_size = pkt_len - rsize - pad_len;
    apkt->pad_size    = pad_len;

    return 0;
err:
    return -1;
}

static int _asf_read_payload(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = -1;
    int sn, len = 0, rsize = 1;
    stream_cls_t *s       = o->s;
    struct asf_priv *priv = o->priv;
    asf_packet_t *apkt    = &priv->apkt;

    apkt->seg_idx++;
    sn = stream_r8(s);
    if (!((sn & 0x7f) == priv->sid)) {
        LOGE(TAG, "invalid sid, sn = %d", sn);
        goto err;
    }

    GET_VAL_2BITS(apkt->pro_flag >> 4, apkt->oid, 0);
    rsize += GET_LEN_2BITS(apkt->pro_flag >> 4);
    GET_VAL_2BITS(apkt->pro_flag >> 2, apkt->frag_offset, 0);
    rsize += GET_LEN_2BITS(apkt->pro_flag >> 2);
    GET_VAL_2BITS(apkt->pro_flag, apkt->repl_size, 0);
    rsize += GET_LEN_2BITS(apkt->pro_flag);

    if (apkt->repl_size >= 8) {
        apkt->obj_size = stream_r32le(s);
        stream_skip(s, 4); // frag timestamp
        rsize += apkt->repl_size;
    } else {
        stream_skip(s, apkt->repl_size);
        rsize += apkt->repl_size;
    }

    if (apkt->obj_size <= 0) {
        LOGE(TAG, "invalid obj_size");
        goto err;
    }

    if (apkt->len_flag & 0x1) {
        GET_VAL_2BITS(apkt->pl_flag >> 6, apkt->frag_size, 0);
    } else {
        apkt->frag_size = apkt->remain_size - rsize;
    }
    CHECK_RET_TAG_WITH_GOTO(apkt->frag_size > 0, err);

    rc = avpacket_grow(pkt, apkt->obj_size);
    if (rc < 0) {
        LOGE(TAG, "avpacket resize fail. pkt->size = %d, framesize = %d", pkt->size, apkt->frag_size);
        goto err;
    }

    //FIXME:
    if (apkt->frag_size + apkt->frag_offset <= apkt->obj_size && pkt->len + apkt->frag_size <= pkt->size) {
        len = stream_read(o->s, pkt->data + pkt->len, apkt->frag_size);
        if (len != apkt->frag_size) {
            LOGE(TAG, "stream read err, len = %d, diff = %d, frame size = %d", len, apkt->frag_size);
            goto err;
        }
        pkt->len += len;
    } else {
        LOGE(TAG, "read payload fail. pkt: size = %d, len = %d. apkt: offset = %d, obj = %d, frag size = %d", pkt->size, pkt->len, apkt->frag_offset, apkt->obj_size, apkt->frag_size);
        goto err;
    }
    if (priv->iduration > 0 && priv->data_size > 0 && priv->start_pos > 0) {
        pkt->pts = apkt->ts * o->time_scale / 1000;
    }
    //printf("===>>>pkt pos = %10d, len = %10d\n", priv->start_pos, len);

    return pkt->len == apkt->obj_size ? 0 : 1;
err:
    return -1;
}

static int _asf_parse_payload(demux_cls_t *o, avpacket_t *pkt)
{
    int rc;
    stream_cls_t *s       = o->s;
    struct asf_priv *priv = o->priv;
    asf_packet_t *apkt    = &priv->apkt;

    if (apkt->seg_idx >= apkt->nb_segments) {
        /* read the next pkt */
        if (apkt->nb_segments)
            stream_seek(s, apkt->pos + priv->packet_size, SEEK_SET);
        return 1;
    }
    rc = _asf_read_payload(o, pkt);

    return rc;
}

static int _demux_asf_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = -1;
    int eof;
    stream_cls_t *s       = o->s;
    struct asf_priv *priv = o->priv;
    asf_packet_t *apkt    = &priv->apkt;

    while (!stream_is_eof(s)) {
        rc = _asf_parse_payload(o, pkt);
        if (rc == 0) {
            return pkt->len;
        } else if (rc > 0) {
            memset(apkt, 0, sizeof(asf_packet_t));
            rc = _asf_get_packet(o);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        } else {
            goto err;
        }
    }

err:
    eof = stream_is_eof(s);
    rc  = eof ? 0 : rc;
    LOGI(TAG, "read packet may be eof. eof = %d, rc = %d, url = %s", eof, rc, stream_get_url(s));
    return rc;
}

//FIXME:
static int _demux_asf_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    uint64_t new_pos;
    struct asf_priv *priv = o->priv;
    size_t packet_size    = priv->packet_size;
    asf_packet_t *apkt    = &priv->apkt;

    if (!(timestamp < priv->iduration && priv->data_size > 0 && priv->start_pos > 0)) {
        LOGE(TAG, "seek failed. ts = %llu, idu = %llu, data_size = %d, spos = %d",
             timestamp, priv->iduration, priv->data_size, priv->start_pos);
        return -1;
    }

    new_pos  = (uint64_t)((1.0 * timestamp / priv->iduration) * priv->data_size) / packet_size * packet_size;
    new_pos += priv->start_pos;
    rc = stream_seek(o->s, new_pos, SEEK_SET);
    memset(apkt, 0, sizeof(asf_packet_t));

    return rc;
}

static int _demux_asf_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_asf = {
    .name            = "asf",
    .type            = AVFORMAT_TYPE_ASF,
    .extensions      = { "asf", "wma", NULL },

    .read_probe      = _demux_asf_probe,

    .open            = _demux_asf_open,
    .close           = _demux_asf_close,
    .read_packet     = _demux_asf_read_packet,
    .seek            = _demux_asf_seek,
    .control         = _demux_asf_control,
};
#endif

