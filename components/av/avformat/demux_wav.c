/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_WAV) && CONFIG_DEMUXER_WAV
#include "avformat/avformat_utils.h"
#include "avformat/riff_rw.h"
#include "avformat/demux_cls.h"
#include "stream/stream.h"

#define TAG                    "demux_wav"

struct wav_priv {
    uint8_t                    is_compress;
    uint32_t                   data_size;
    uint32_t                   pos;
    uint32_t                   fact_size;
    uint32_t                   block_align;
    uint32_t                   sec_size;
    uint32_t                   frame_size;
    int32_t                    start_pos;
    uint64_t                   iduration;  ///< inner duration, base time_base
};

/**
 * @brief  read wav chunk header
 * @param  [in] s
 * @param  [in] name
 * @param  [in] size
 * @return 0 on ok, -1 on stream err, 1 on header not match
 */
static int _read_chunk_header(stream_cls_t *s, const char *name, unsigned int *size)
{
    int rc;
    char buf[8];

    rc = stream_read(s, (uint8_t*)buf, 8);
    if (rc != 8) {
        LOGE(TAG, "read chunk header err, rc = %d", rc);
        return -1;
    }

    *size = byte_r32le((const uint8_t*)buf + 4);
    if (memcmp(buf, name, 4)) {
        //LOGE(TAG, "wave wrong header");
        return 1;
    }

    return 0;
}

//TODO
static int _find_chunk(stream_cls_t *s, const char *name, unsigned int *size)
{
    int rc = 0;

    while (rc == 0) {
        rc = _read_chunk_header(s, name, size);
        if (rc != 1) {
            /* err happen or read ok */
            return rc;
        }
        rc = stream_seek(s, *size, SEEK_CUR);
    }

    return rc;
}

static int _demux_wav_probe(const avprobe_data_t *pd)
{
    uint8_t *buf = pd->buf;
    int buf_size = pd->buf_size;

    if (buf_size < 16)
        return 0;

    if (strncmp((const char*)buf, "RIFF", 4))
        return 0;

    if (strncmp((const char*)buf + 8, "WAVEfmt", 7))
        return 0;

    return AVPROBE_SCORE_MAX;
}

static int _demux_wav_open(demux_cls_t *o)
{
    int rc;
    char buf[4];
    struct wav_priv *priv;
    int start_pos, ext_size, size;
    unsigned int riff_size, fmt_size = 0;
    unsigned int format_tag, channels, rate, bits, isfloat;

    priv = av_zalloc(sizeof(struct wav_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    rc = _read_chunk_header(o->s, "RIFF", &riff_size);
    if (rc)
        goto err;
    rc = stream_read(o->s, (uint8_t*)buf, 4);
    if (rc != 4 || memcmp(buf, "WAVE", 4) != 0) {
        LOGE(TAG, "wave wrong header");
        rc = -1;
        goto err;
    }

    rc = _find_chunk(o->s, "fmt ", &fmt_size);
    if (rc || (fmt_size < 16)) {
        LOGE(TAG, "size of \"fmt \" chunk is invalid (%u), rc = %d\n", fmt_size, rc);
        goto err;
    }

    start_pos  = stream_tell(o->s);
    format_tag = stream_r16le(o->s);
    channels   = stream_r16le(o->s);
    rate       = stream_r32le(o->s);
    stream_skip(o->s, 4); // byte rate
    priv->block_align = stream_r16le(o->s);
    bits = stream_r16le(o->s);
    if (format_tag == WAV_FORMAT_EXTENSIBLE) {
        ext_size = stream_r16le(o->s);
        if (ext_size < 22) {
            LOGE(TAG, "ext_size invalid, %d", ext_size);
            goto err;
        }
        stream_skip(o->s, 2); // valid bits
        stream_skip(o->s, 4); // channel mask
        format_tag = stream_r16le(o->s);
    }
    size = fmt_size - (stream_tell(o->s) - start_pos);
    if (size > 0) {
        stream_skip(o->s, size);
    }

    switch (format_tag) {
    case WAV_FORMAT_PCM:
    case WAV_FORMAT_FLOAT:
        o->ash.id = AVCODEC_ID_RAWAUDIO;
        break;
    case WAV_FORMAT_ADPCM_MS:
        bits              *= 4;
        o->ash.id          = AVCODEC_ID_ADPCM_MS;
        priv->is_compress  = 1;
        break;
    case WAV_FORMAT_ALAW:
        bits              *= 2;
        o->ash.id          = AVCODEC_ID_ALAW;
        //FIXME: block align is 1, too small
        priv->block_align *= 512;
        priv->is_compress  = 1;
        break;
    case WAV_FORMAT_MULAW:
        bits              *= 2;
        o->ash.id          = AVCODEC_ID_MULAW;
        priv->block_align *= 512;
        priv->is_compress  = 1;
        break;
    default:
        LOGE(TAG, "unsupported format tag %u", format_tag);
        goto err;
    }
    if (priv->is_compress) {
        //must contains fact-chunk
        rc = _find_chunk(o->s, "fact", &priv->fact_size);
        if (rc) {
            LOGE(TAG, "compress formats, fact not find");
            goto err;
        }
        priv->fact_size  = stream_r32le(o->s);
    }

    if ((bits != 8 && bits != 16 && bits != 24 && bits != 32 && bits != 64) || channels < 1) {
        LOGE(TAG, "unsupported wav. bits = %u, channels = %u\n", bits, channels);
        goto err;
    }

    isfloat            = format_tag == WAV_FORMAT_FLOAT ? 1 : 0;
    o->ash.block_align = priv->block_align;
    o->ash.sf          = sf_make_channel(channels) | sf_make_rate(rate) | sf_make_bit(bits) | sf_make_signed(bits > 8) | sf_make_float(isfloat);

    rc = _find_chunk(o->s, "data", &priv->data_size);
    if (rc)
        goto err;

    priv->pos         = 0;
    priv->sec_size    = sf_get_bps(o->ash.sf) / 8;
    priv->frame_size  = sf_get_frame_size(o->ash.sf);
    priv->data_size  -= priv->data_size % sf_get_frame_size(o->ash.sf);
    priv->start_pos   = stream_tell(o->s);
    o->time_scale     = AV_FLICK_BASE / rate;
    o->duration       = priv->fact_size ? priv->fact_size * 1000 / rate : 1.0 * (priv->data_size / priv->frame_size) / rate * 1000;
    priv->iduration   = o->duration / 1000.0 * o->time_scale;
    o->priv           = priv;

    return 0;
err:
    av_free(priv);
    return -1;
}

static int _demux_wav_close(demux_cls_t *o)
{
    struct wav_priv *priv = o->priv;

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _demux_wav_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc, len;
    size_t flen;
    sf_t sf               = o->ash.sf;
    struct wav_priv *priv = o->priv;

    if (priv->pos == priv->data_size) {
        goto eof;
    }

    flen = priv->is_compress ? priv->block_align : CONFIG_AV_SAMPLE_NUM_PER_FRAME_MAX * sf_get_frame_size(sf);
    if (pkt->size < flen) {
        rc = avpacket_grow(pkt, flen);
        if (rc < 0) {
            LOGE(TAG, "avpacket resize fail. pkt->size = %d, framesize = %d", pkt->size, flen);
            return -1;
        }
    }
    flen = (flen > priv->data_size - priv->pos) ? priv->data_size - priv->pos : flen;
    len = stream_read(o->s, pkt->data, flen);
    if (len == -1) {
        LOGE(TAG, "read error\n");
        return -1;
    }
    if (len == 0) {
        goto eof;
    }

    pkt->len = len;
    if (priv->iduration > 0 && priv->start_pos >= 0) {
        pkt->pts = 1.0 * priv->pos / priv->data_size * priv->iduration;
    }
    priv->pos += len;
    return len;
eof:
    return 0;
}

static int _demux_wav_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    size_t new_pos, pos;
    struct wav_priv *priv = o->priv;

    if (!(timestamp < priv->iduration && stream_get_size(o->s) > 0 && priv->start_pos >= 0)) {
        LOGE(TAG, "seek failed. ts = %llu, idu = %llu, file_size = %d, spos = %d",
             timestamp, priv->iduration, stream_get_size(o->s), priv->start_pos);
        return -1;
    }

    pos       = (1.0 * timestamp / priv->iduration) * priv->data_size;
    pos       = priv->is_compress ? AV_ALIGN_SIZE(pos, priv->block_align) : AV_ALIGN_SIZE(pos, priv->frame_size);
    new_pos   = priv->start_pos + pos;
    rc        = stream_seek(o->s, new_pos, SEEK_SET);
    priv->pos = rc == 0 ? pos : priv->pos;

    return rc;
}

static int _demux_wav_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_wav = {
    .name            = "wav",
    .type            = AVFORMAT_TYPE_WAV,
    .extensions      = { "wav", NULL },

    .read_probe      = _demux_wav_probe,

    .open            = _demux_wav_open,
    .close           = _demux_wav_close,
    .read_packet     = _demux_wav_read_packet,
    .seek            = _demux_wav_seek,
    .control         = _demux_wav_control,
};
#endif


