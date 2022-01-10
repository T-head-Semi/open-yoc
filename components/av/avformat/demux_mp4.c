/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DEMUXER_MP4) && CONFIG_DEMUXER_MP4 && defined(CONFIG_USING_TLS)
#include "avutil/av_config.h"
#include "avutil/misc.h"
#include "avutil/byte_rw.h"
#include "avutil/url_parse.h"
#include "stream/stream.h"
#include "avformat/demux_cls.h"
#include "avformat/mp4_types.h"
#include "avformat/avformat_utils.h"
#include "avformat/mp3_rw.h"
#include <mbedtls/aes.h>

#define TAG                    "demux_mp4"

#define AES_128_BITS           (128)
#define AES_128_BYTES          (16)

const avcodec_tag_t g_codec_tags_audio[] = {
    { AVCODEC_ID_AAC,          TAG_VAL('m', 'p', '4', 'a') },
    { AVCODEC_ID_MP3,          TAG_VAL('.', 'm', 'p', '3') },
    { AVCODEC_ID_UNKNOWN,      0                           },
};

const avcodec_tag_t g_mp4_obj_type[] = {
    { AVCODEC_ID_AAC,          0x40 },
    { AVCODEC_ID_AAC,          0x66 },          /* MPEG2 AAC Main */
    { AVCODEC_ID_AAC,          0x67 },          /* MPEG2 AAC Low */
    { AVCODEC_ID_AAC,          0x68 },          /* MPEG2 AAC SSR */
    { AVCODEC_ID_MP3,          0x69 },          /* MPEG2 MP3 */
    { AVCODEC_ID_MP3,          0x6b },          /* MPEG1 MP3 */
    { AVCODEC_ID_UNKNOWN,      0x0  },
};

#if CONFIG_AV_MP4_IDX_OPT
#define AUDIO_SAMPLE_SIZE_TYPE  int16_t
#else
#define AUDIO_SAMPLE_SIZE_TYPE  int32_t
#endif

typedef struct index_entry {
    int32_t                    pos;
    int32_t                    size;
    int64_t                    timestamp;       ///< base the time_scale
} index_entry_t;

#if CONFIG_AV_MP4_IDX_OPT
#define INDEX_ENTRY_MEM_MAX    (0x8000)
#define IDX_NB_PER_SEGMENT     (INDEX_ENTRY_MEM_MAX / sizeof(index_entry_t))

typedef struct segment_index {
    int32_t                    start;
    int32_t                    end;
    int64_t                    start_ts;       ///< timestamp
    int64_t                    end_ts;         ///< timestamp
} seg_idx_t;
#endif

typedef struct encrypt_info {
    uint8_t                    *iv;
    uint32_t                   iv_size;
} encrypt_info_t;

typedef struct aes_crypt {
    mbedtls_aes_context        ctx;
    size_t                     offset;
    uint8_t                    iv[AES_128_BYTES];
    uint8_t                    block[AES_128_BYTES];
} aes_crypt_t;

struct mp4_priv {
    int8_t                     isom;
    int8_t                     find_ftyp;
    int8_t                     find_moov;
    int8_t                     find_mdat;
    int8_t                     not_atrak;

    uint32_t                   time_scale;               ///< for the media
    uint64_t                   duration;                 ///< for the media

    uint32_t                   chunk_count;
    uint32_t                   stts_count;
    uint32_t                   stsc_count;
    uint32_t                   sample_count;
    int32_t                    *chunk_offsets;
    AUDIO_SAMPLE_SIZE_TYPE     *sample_sizes;
    mp4_stts_t                 *stts_data;
    mp4_stsc_t                 *stsc_data;

    uint32_t                   cur_sample;
    index_entry_t              *indexes;
    uint32_t                   nb_indexes;
#if CONFIG_AV_MP4_IDX_OPT
    seg_idx_t                  *seg_idxes;
    int32_t                    nb_seg_idxes;
    int32_t                    cur_seg_idx;
#endif

    avcodec_id_t               id;
    uint8_t                    major_brand[5];
    int32_t                    rate;
    int32_t                    channel;
    uint32_t                   frame_size;
    uint32_t                   sample_size;              ///< used for stsd
    uint32_t                   szsample_size;            ///< used for stsz

    /* used for cenc-aes-ctr */
    int32_t                    cenc_encrypted;
    uint8_t                    per_sample_iv_size;
    encrypt_info_t             *encrypt_samples;         ///< array. the number must equals nb_indexes
    encrypt_info_t             *default_encrypt_sample;
    aes_crypt_t                *aes_param;
    uint8_t                    cenc_key[AES_128_BYTES];
};

typedef int (*atom_parser_t)(demux_cls_t *o, mp4_atom_t *atom);
typedef struct {
    uint32_t                   type;
    atom_parser_t              parser;
} mp4_parser_t;

static int _mp4_read_atom(demux_cls_t *o, mp4_atom_t *atom);
#if CONFIG_AV_MP4_IDX_OPT
static int _mp4_create_indexes(demux_cls_t *o, int start_idx);
#else
static int _mp4_create_indexes(demux_cls_t *o);
#endif
static int _demux_mp4_read_packet(demux_cls_t *o, avpacket_t *pkt);

static void _mp4_read_close(demux_cls_t *o)
{
    struct mp4_priv *priv = o->priv;

    av_freep((char**)&priv->chunk_offsets);
    av_freep((char**)&priv->sample_sizes);
    av_freep((char**)&priv->stsc_data);
    av_freep((char**)&priv->stts_data);
#if CONFIG_AV_MP4_IDX_OPT
    av_freep((char**)&priv->seg_idxes);
#endif
    if (priv->cenc_encrypted) {
        if (priv->encrypt_samples) {
            av_freep((char **)&priv->encrypt_samples[0].iv);
            av_freep((char **)&priv->encrypt_samples);
        }

        if (priv->aes_param) {
            mbedtls_aes_free(&priv->aes_param->ctx);
            av_freep((char **)&priv->aes_param);
        }

        if (priv->default_encrypt_sample && priv->default_encrypt_sample->iv) {
            av_freep((char **)&priv->default_encrypt_sample->iv);
        }

        av_freep((char **)&priv->default_encrypt_sample);
    }

    av_freep((char**)&priv->indexes);
}

static int _mp4_read_hdlr(demux_cls_t *o, mp4_atom_t *atom)
{
    uint32_t type;
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int32_t start_pos, size;

    start_pos = stream_tell(s);
    stream_r8(s);
    stream_r24be(s); /* flags */

    stream_skip(s, 4);
    type = stream_r32le(s);

    switch (type) {
    case ATOMID('v', 'i', 'd', 'e'):
    case ATOMID('s', 'u', 'b', 'p'):
    case ATOMID('c', 'l', 'c', 'p'):
        LOGD(TAG, "not a audio track, skip it, type = %s", four_2_str(type));
        priv->not_atrak = 1;
        break;
    default:
        break;
    }
    size = stream_tell(s) - start_pos;
    if (size > 0) {
        stream_skip(s, size);
        return 0;
    }

    return -1;
}

static int _mp4_read_mdat(demux_cls_t *o, mp4_atom_t *atom)
{
    struct mp4_priv *priv = o->priv;

    if (atom->size <= 0) {
        LOGE(TAG, "mdata is err. size = %d", atom->size);
        return -1;
    }

    priv->find_mdat = 1;
    if (!priv->find_moov) {
        LOGI(TAG, "may be moov behind mdat!");
    }

    return 0;
}

static int _mp4_read_ftyp(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;

    if ((!priv->find_ftyp) && (atom->size <= ATOM_FTYP_SIZE_MAX)) {
        stream_read(s, priv->major_brand, 4);
        priv->isom = 1;

        stream_skip(s, atom->size - 4);
        priv->find_ftyp = 1;
        return 0;
    }

    return -1;
}

static int _mp4_read_moov(demux_cls_t *o, mp4_atom_t *atom)
{
    struct mp4_priv *priv = o->priv;

    if ((!priv->find_moov) && priv->find_ftyp) {
        priv->find_moov = 1;
        return _mp4_read_atom(o, atom);
    }

    return -1;
}

static int _mp4_read_mdhd(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int version;

    if (!priv->find_moov) {
        return -1;
    }
    version = stream_r8(s);
    stream_r24be(s); /* flags */

    if (version == 1) {
        stream_r64be(s);
        stream_r64be(s);
    } else {
        stream_r32be(s);
        stream_r32be(s);
    }

    o->time_scale = stream_r32be(s);
    o->duration   = (version == 1) ? stream_r64be(s) : stream_r32be(s);
    if (!(o->time_scale > 0 && o->duration > 0)) {
        LOGE(TAG, "time_scale or duration is err. time_scale = %u, duration = %llu", o->time_scale, o->duration);
        return -1;
    }
    o->duration = o->duration * 1000 / o->time_scale; /* ms */
    stream_r16be(s);    /* lan */
    stream_r16be(s);    /* quality */

    return 0;
}

static int _mp4_read_mvhd(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int version;

    if (!priv->find_moov) {
        return -1;
    }

    version = stream_r8(s);
    stream_r24be(s); /* flags */

    if (version == 1) {
        stream_r64be(s);
        stream_r64be(s);
        priv->time_scale = stream_r32be(s); /* time scale */
        priv->duration   = stream_r64be(s); /* duration */
    } else {
        stream_r32be(s);
        stream_r32be(s);
        /* max duration for the media(a/v), don't use this now */
        priv->time_scale = stream_r32be(s); /* time scale */
        priv->duration   = stream_r32be(s); /* duration */
    }

    stream_r32be(s);    /* preferred scale */
    stream_r16be(s);    /* preferred volume */
    stream_skip(s, 10); /* reserved */
    stream_skip(s, 36); /* display matrix */
    stream_skip(s, 24); /* pre_defined */
    stream_r32be(s);    /* next track ID */

    return 0;
}

static int _mp4_read_stco(demux_cls_t *o, mp4_atom_t *atom)
{
    int rc;
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int i, entries;

    stream_r8(s); /* version */
    stream_r24be(s); /* flags */
    entries = stream_r32be(s);
    if (!((entries > 0) && ((entries * sizeof(int64_t)) < INT32_MAX))) {
        LOGE(TAG, "stco entries is err. entries = %d", entries);
        return -1;
    }

    priv->chunk_offsets = av_zalloc(entries * sizeof(int64_t));
    if (!priv->chunk_offsets) {
        LOGE(TAG, "stco err, oom. ");
        return -1;
    }

    for (i = 0; i < entries; i++) {
        priv->chunk_offsets[i] = stream_r32be(s);
    }
    priv->chunk_count = i;

#if CONFIG_AV_MP4_IDX_OPT
    rc = _mp4_create_indexes(o, 0);
#else
    rc = _mp4_create_indexes(o);
#endif
    if (rc < 0) {
        LOGE(TAG, "mp4 create indexes fail");
        return -1;
    }

    return 0;
}

static int _mp4_read_descr(stream_cls_t *s, int *tag)
{
    int len = 0, count = 4, c;

    *tag = stream_r8(s);
    while (count--) {
        c = stream_r8(s);
        len = (len << 7) | (c & 0x7f);
        if (!(c & 0x80))
            break;
    }

    return len;
}

static void _mp4_parse_es_descr(stream_cls_t *s)
{
    /* ES_Descriptor */
    int flags, len;

    stream_r16be(s);
    flags = stream_r8(s);

    if (flags & 0x80) {
        /* streamDependenceFlag */
        stream_r16be(s);
    }

    if (flags & 0x40) {
        /* URL_Flag */
        len = stream_r8(s);
        stream_skip(s, len);
    }

    if (flags & 0x20) {
        /* OCRstreamFlag */
        stream_r16be(s);
    }
}

static int _mp4_get_extradata(demux_cls_t *o, size_t size)
{
    int ret;
    stream_cls_t *s       = o->s;

    av_freep((char**)&o->ash.extradata);
    o->ash.extradata_size = 0;

    o->ash.extradata = av_malloc(size);
    if (NULL == o->ash.extradata) {
        LOGE(TAG, "read descr fail. oom");
        return -1;
    }

    ret = stream_read(s, o->ash.extradata, size);
    if (ret != size) {
        LOGE(TAG, "read descr fail, ret = %d, size = %d", ret, size);
        av_freep((char**)&o->ash.extradata);
        return -1;
    }
    o->ash.extradata_size = size;

    return 0;
}

static int _mp4_read_dec_config_descr(demux_cls_t *o)
{
#define MP4_SPECIFIC_DESC_SIZE_MAX (1024)
    int ret;
    int len, tag, obj_indication;
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;

    obj_indication = stream_r8(s);
    stream_r8(s);    /* stream type */
    stream_r24be(s); /* buffer size db */
    stream_r32be(s); /* max bitrate */
    stream_r32be(s); /* avg bitrate */

    priv->id = get_codec_id(g_mp4_obj_type, obj_indication);
    len = _mp4_read_descr(s, &tag);
    if (tag == MP4_DEC_SPECIFIC_DESCR_TAG) {
        if (!((len > 0 ) && (len < MP4_SPECIFIC_DESC_SIZE_MAX))) {
            LOGE(TAG, "mp4 specific header len=%d", len);
            return -1;
        }

        ret = _mp4_get_extradata(o, len);
        if (ret < 0) {
            LOGE(TAG, "mp4 get extradata fail");
            return -1;
        }
    }

    return 0;
}

/* see 14496-1 */
static int _mp4_read_esds(demux_cls_t *o, mp4_atom_t *atom)
{
    int tag;
    stream_cls_t *s       = o->s;
    //struct mp4_priv *priv = o->priv;

    stream_r8(s); /* version */
    stream_r24be(s); /* flags */

    _mp4_read_descr(s, &tag);
    if (tag == MP4_ES_DESCR_TAG) {
        _mp4_parse_es_descr(s);
    } else {
        stream_r16be(s); /* ID */
    }

    _mp4_read_descr(s, &tag);
    if (tag == MP4_DEC_CONFIG_DESCR_TAG) {
        _mp4_read_dec_config_descr(o);
    }

    return 0;
}

static void _parse_stsd_audio(demux_cls_t *o)
{
    int version;
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;

    version = stream_r16be(s);
    stream_r16be(s); /* revision level */
    stream_r32be(s); /* vendor */

    priv->channel     = stream_r16be(s); /* channel count */
    priv->sample_size = stream_r16be(s); /* sample size */

    stream_r16be(s);
    stream_r16be(s);
    priv->rate = ((stream_r32be(s) >> 16));
    if (version == 1) {
        stream_r32be(s); /* reserved */
        stream_r32be(s); /* reserved */
        stream_r32be(s); /* reserved */
        stream_r32be(s); /* reserved */
    } else if (version == 2) {
        stream_r32be(s); /* reserved */
        priv->rate    = stream_r64be(s);
        priv->channel = stream_r32be(s);
        stream_r32be(s); /* reserved */
        stream_r32be(s); /* reserved */
        stream_r32be(s); /* reserved */
        stream_r32be(s); /* reserved */
        stream_r32be(s); /* reserved */
    }
}

static int _mp4_read_wave(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;

    if (atom->size > 8) {
        return _mp4_read_atom(o, atom);
    } else {
        stream_skip(s, atom->size);
    }

    return 0;
}

static int _mp4_read_stsd(demux_cls_t *o, mp4_atom_t *atom)
{
    int entries;
    int32_t start_pos, size, format;
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;

    stream_r8(s); /* version */
    stream_r24be(s); /* flags */
    entries = stream_r32be(s);
    if (entries != 1) {
        LOGE(TAG, "entries = %d\n", entries);
        return -1;
    }

    start_pos = stream_tell(s);
    size      = stream_r32be(s);
    format    = stream_r32le(s);

    if (size >= 16) {
        stream_r32be(s); /* reserved */
        stream_r16be(s); /* reserved */
        stream_r16be(s); /* dref id */
    } else if (size <= 7) {
        LOGE(TAG, "invalid size = %d\n", size);
        return -1;
    }

    priv->id = get_codec_id(g_codec_tags_audio, format);
    //FIXME: the priv id may be parsed in frma atom box
    _parse_stsd_audio(o);

    size = size - (stream_tell(s) - start_pos);
    if (size > 8) {
        /* read extra atoms, esds */
        mp4_atom_t extra_atom;

        extra_atom.size = size;
        extra_atom.type = byte_r32le((const uint8_t*)"stsd");
        return _mp4_read_atom(o, &extra_atom);
    } else if (size > 0) {
        stream_skip(s, size);
    }

    return 0;
}

static int _mp4_read_stsc(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int i, entries;

    stream_r8(s); /* version */
    stream_r24be(s); /* flags */
    entries = stream_r32be(s);
    if ((entries * sizeof(mp4_stsc_t) + 8) > atom->size) {
        LOGE(TAG, "stsc entries is err. entries = %d, atom->size = %lld", entries, atom->size);
        return -1;
    }

    priv->stsc_data = av_zalloc(entries * sizeof(mp4_stsc_t));
    if (!priv->stsc_data) {
        LOGE(TAG, "stts data malloc fail, oom.");
        return -1;
    }

    for (i = 0; i < entries; i++) {
        priv->stsc_data[i].first = stream_r32be(s);
        priv->stsc_data[i].count = stream_r32be(s);
        priv->stsc_data[i].id    = stream_r32be(s);
    }
    priv->stsc_count = i;

    return 0;
}

static int _mp4_read_stsz(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int i, entries, sample_size;

    stream_r8(s); /* version */
    stream_r24be(s); /* flags */
    sample_size         = stream_r32be(s);
    priv->sample_size   = priv->sample_size > 0 ? priv->sample_size : sample_size;
    priv->szsample_size = sample_size;
    if (sample_size) {
        LOGD(TAG, "stsz: all sample size is equal. sample_size = %d", sample_size);
        return 0;
    }
    entries = stream_r32be(s);
    if (!((entries > 0) && ((entries * sizeof(int32_t)) < INT32_MAX))) {
        LOGE(TAG, "stsz entries is err. entries = %d", entries);
        return -1;
    }

    priv->sample_count = entries;
    /* FIXME: sample_sizes may be too large */
    priv->sample_sizes = av_zalloc(entries * sizeof(AUDIO_SAMPLE_SIZE_TYPE));
    if (!priv->sample_sizes) {
        LOGE(TAG, "stsz fail, oom.");
        return -1;
    }

    for (i = 0; i < entries; i++) {
        priv->sample_sizes[i] = (AUDIO_SAMPLE_SIZE_TYPE)stream_r32be(s);
    }

    return 0;
}

static int _mp4_read_stts(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int i, entries;

    stream_r8(s); /* version */
    stream_r24be(s); /* flags */
    entries = stream_r32be(s);
    if (!((entries > 0) && ((entries * sizeof(mp4_stts_t)) < INT32_MAX))) {
        LOGE(TAG, "stts entries is err. entries = %d", entries);
        return -1;
    }

    av_free(priv->stts_data);
    priv->stts_data = av_zalloc(entries * sizeof(mp4_stts_t));
    if (!priv->stts_data) {
        LOGE(TAG, "stts data malloc fail, oom.");
        return -1;
    }

    for (i = 0; i < entries; i++) {
        int duration;
        int count;

        count    = stream_r32be(s);
        duration = stream_r32be(s);
        if (!((count > 0) && (duration > 0))) {
            LOGE(TAG, "stts err. count = %d, duration = %d", count, duration);
            return -1;
        }

        priv->stts_data[i].count     = count;
        priv->stts_data[i].duration  = duration;
        priv->sample_count          += count;
    }
    priv->stts_count = i;

    return 0;
}

#if CONFIG_AV_MP4_IDX_OPT
static int _mp4_create_indexes(demux_cls_t *o, int start_idx)
{
    uint32_t i, j, cur_offset;
    index_entry_t *indexes = NULL;
    struct mp4_priv *priv  = o->priv;
    seg_idx_t *seg_idxes   = priv->seg_idxes;
    int64_t cur_dts        = 0;
    uint32_t stsc_index    = 0;
    uint32_t cur_sample    = 0, sample_size, nb_idxes;
    uint32_t stts_sample   = 0, stts_index = 0, nb_indexes = 0;

    //printf("===>>>start idx = %d\n", start_idx);
    nb_idxes = priv->sample_count > IDX_NB_PER_SEGMENT ? IDX_NB_PER_SEGMENT : priv->sample_count;
    if (priv->indexes) {
        indexes = priv->indexes;
        memset(priv->indexes, 0, sizeof(index_entry_t) * nb_idxes);
    } else {
        indexes = av_zalloc(sizeof(index_entry_t) * nb_idxes);
        if (!indexes) {
            LOGE(TAG, "alloc fail, oom, sample_count = %d", nb_idxes);
            return -1;
        }

        priv->nb_seg_idxes = (priv->sample_count + (IDX_NB_PER_SEGMENT - 1)) / IDX_NB_PER_SEGMENT;
        priv->seg_idxes    = av_zalloc(sizeof(seg_idx_t) * priv->nb_seg_idxes);
        for (i = 0; i < priv->nb_seg_idxes; i++) {
            priv->seg_idxes[i].start = i * IDX_NB_PER_SEGMENT;
            priv->seg_idxes[i].end   = (i + 1) * IDX_NB_PER_SEGMENT - 1;
        }
        priv->seg_idxes[i - 1].end = priv->sample_count - 1;
    }

    for (i = 0; i < priv->chunk_count; i++) {
        cur_offset  = priv->chunk_offsets[i];
        while ((stsc_index + 1 < priv->stsc_count) && (i + 1 == priv->stsc_data[stsc_index + 1].first))
            stsc_index++;

        for (j = 0; j < priv->stsc_data[stsc_index].count; j++) {
            index_entry_t *e;
            if (nb_indexes >= priv->sample_count) {
                LOGE(TAG, "wrong sample count\n");
                goto err;
            }

            sample_size  = priv->szsample_size ? priv->szsample_size : priv->sample_sizes[cur_sample++];
            if (nb_indexes >= start_idx && nb_indexes < start_idx + IDX_NB_PER_SEGMENT) {
                e            = &indexes[nb_indexes - start_idx];
                e->pos       = cur_offset;
                e->size      = sample_size;
                e->timestamp = cur_dts;
            }

            if (seg_idxes) {
                if (nb_indexes >= start_idx + IDX_NB_PER_SEGMENT) {
                    /* seg_idxes constructed already */
                    goto quit;
                }
            } else {
                int seg_index = nb_indexes / IDX_NB_PER_SEGMENT;

                if (nb_indexes == priv->seg_idxes[seg_index].start)
                    priv->seg_idxes[seg_index].start_ts = cur_dts;
                if (nb_indexes == priv->seg_idxes[seg_index].end)
                    priv->seg_idxes[seg_index].end_ts = cur_dts;
            }
            nb_indexes++;
            cur_offset += sample_size;

            cur_dts += priv->stts_data[stts_index].duration;
            stts_sample++;
            if (stts_index + 1 < priv->stts_count && stts_sample == priv->stts_data[stts_index].count) {
                stts_sample = 0;
                stts_index++;
            }
        }
    }
quit:
    priv->indexes    = indexes;
    priv->nb_indexes = priv->sample_count;

    return 0;
err:
    av_free(indexes);
    return -1;
}
#else
static int _mp4_create_indexes(demux_cls_t *o)
{
    uint32_t i, j, cur_offset;
    index_entry_t *indexes = NULL;
    struct mp4_priv *priv  = o->priv;
    int64_t cur_dts        = 0;
    uint32_t stsc_index    = 0;
    uint32_t cur_sample    = 0, sample_size;
    uint32_t stts_sample   = 0, stts_index = 0;

    indexes = av_zalloc(sizeof(index_entry_t) * priv->sample_count);
    if (!indexes) {
        /* FIXME: sample count may be too large for big m4a */
        LOGE(TAG, "alloc fail, oom, sample_count = %d\n", priv->sample_count);
        return -1;
    }

    for (i = 0; i < priv->chunk_count; i++) {
        cur_offset  = priv->chunk_offsets[i];
        while ((stsc_index + 1 < priv->stsc_count) && (i + 1 == priv->stsc_data[stsc_index + 1].first))
            stsc_index++;

        for (j = 0; j < priv->stsc_data[stsc_index].count; j++) {
            index_entry_t *e;
            if (cur_sample >= priv->sample_count) {
                LOGE(TAG, "wrong sample count\n");
                goto err;
            }

            sample_size  = priv->szsample_size ? priv->szsample_size : priv->sample_sizes[cur_sample++];
            e            = &indexes[priv->nb_indexes++];
            e->pos       = cur_offset;
            e->size      = sample_size;
            e->timestamp = cur_dts;
            cur_offset  += sample_size;

            cur_dts += priv->stts_data[stts_index].duration;
            stts_sample++;
            if (stts_index + 1 < priv->stts_count && stts_sample == priv->stts_data[stts_index].count) {
                stts_sample = 0;
                stts_index++;
            }
        }
    }
    priv->indexes = indexes;

    return 0;
err:
    av_free(indexes);
    return -1;
}
#endif

static int _mp4_read_trak(demux_cls_t *o, mp4_atom_t *atom)
{
    int ret = -1;
    struct mp4_priv *priv = o->priv;

    if (!priv->find_moov) {
        return -1;
    }

    if ((ret = _mp4_read_atom(o, atom)) < 0) {
        goto err;
    }
    if (priv->not_atrak == 1) {
        priv->not_atrak = 0;
        ret = 0;
        goto err;
    }

    if (!(priv->chunk_count && priv->stts_count && priv->stsc_count && priv->sample_count)) {
        LOGE(TAG, "chunk_count = %u, stts_count = %u, stsc_count = %u, sample_count = %u, sample_size = %u",
             priv->chunk_count, priv->stts_count, priv->stsc_count, priv->sample_count);
        ret = -1;
        goto err;
    }

err:
#if CONFIG_AV_MP4_IDX_OPT
    if (ret < 0) {
        av_freep((char**)&priv->chunk_offsets);
        av_freep((char**)&priv->sample_sizes);
        av_freep((char**)&priv->stsc_data);
        av_freep((char**)&priv->stts_data);
    }
#else
    av_freep((char**)&priv->chunk_offsets);
    av_freep((char**)&priv->sample_sizes);
    av_freep((char**)&priv->stsc_data);
    av_freep((char**)&priv->stts_data);
#endif

    return ret;
}

static int _mp4_read_tkhd(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;

    //FIXME: just skip
    if (priv->find_moov) {
        stream_skip(s, atom->size);
        return 0;
    }

    return -1;
}

static int _mp4_read_frma(demux_cls_t *o, mp4_atom_t *atom)
{
    uint32_t format;
    avcodec_id_t id;
    stream_cls_t *s = o->s;
    struct mp4_priv *priv = o->priv;

    format   = stream_r32le(s);
    id = get_codec_id(g_codec_tags_audio, format);
    priv->id = (priv->id == AVCODEC_ID_UNKNOWN) ? id : priv->id;
    if (priv->id == AVCODEC_ID_UNKNOWN) {
        LOGE(TAG, "frma id err. format = %x\n", format);
        return -1;
    }

    return 0;
}

static int _mp4_read_tenc(demux_cls_t *o, mp4_atom_t *atom)
{
    stream_cls_t *   s    = o->s;
    struct mp4_priv *priv = o->priv;
    int              is_protected, per_sample_iv_size, default_iv_size;
    int              ret;

    if (!priv->cenc_encrypted) {
        LOGE(TAG, "tenc but not encrypt");
        return -1;
    }

    stream_r8(s);    /* version */
    stream_r24be(s); /* flags */
    stream_r8(s);    /* reserved */
    stream_r8(s);    /* pattern */
    is_protected       = stream_r8(s); /* is_protected */
    per_sample_iv_size = stream_r8(s); /* per sample iv size */
    if (per_sample_iv_size != 0 && per_sample_iv_size != 8 && per_sample_iv_size != 16) {
        return -1;
    }
    priv->per_sample_iv_size = per_sample_iv_size;
    stream_skip(s, 16); // skip red the default key id
    if (is_protected && !per_sample_iv_size) {
        priv->default_encrypt_sample = av_zalloc(sizeof(encrypt_info_t));
        if (!priv->default_encrypt_sample) {
            LOGE(TAG, "tenc malloc err ");
            return -1;
        }

        default_iv_size = stream_r8(s);
        CHECK_RET_TAG_WITH_GOTO(default_iv_size == 8 || default_iv_size == 16, err);

        priv->default_encrypt_sample->iv_size = default_iv_size;
        priv->default_encrypt_sample->iv      = av_zalloc(default_iv_size);
        CHECK_RET_TAG_WITH_GOTO(priv->default_encrypt_sample->iv, err);

        ret = stream_read(s, priv->default_encrypt_sample->iv, default_iv_size);
        CHECK_RET_TAG_WITH_GOTO(ret == default_iv_size, err);
    }
    return 0;

err:
    av_freep((char **)&priv->default_encrypt_sample);
    av_freep((char **)&priv->default_encrypt_sample->iv);
    return -1;
}

static int _mp4_read_senc(demux_cls_t *o, mp4_atom_t *atom)
{
    int rc;
    uint8_t *ives         = NULL;
    stream_cls_t *   s    = o->s;
    struct mp4_priv *priv = o->priv;
    int sample_cnt, hav_subsamples, i;

    if (!priv->cenc_encrypted) {
        LOGE(TAG, "senc but not encrypt");
        return -1;
    }

    stream_r8(s);                     /* version */
    hav_subsamples = stream_r24be(s); /* flags use sub samples*/
    if (hav_subsamples) {
        LOGE(TAG, "sub sample not support");
        return -1;
    }

    sample_cnt = stream_r32be(s);
    if (priv->indexes == NULL || sample_cnt != priv->nb_indexes) {
        LOGE(TAG, "read senc before create indexes");
        return -1;
    }
    if (priv->encrypt_samples) {
        av_freep((char **)&priv->encrypt_samples[0].iv);
        av_freep((char **)&priv->encrypt_samples);
    }

    if (priv->per_sample_iv_size > 0) {
        rc = priv->per_sample_iv_size * sample_cnt;
        if (rc + 8 > atom->size) {
            LOGE(TAG, "format err");
            return -1;
        }

        priv->encrypt_samples = av_zalloc(sample_cnt * sizeof(encrypt_info_t));
        CHECK_RET_TAG_WITH_RET(priv->encrypt_samples, -1);
        ives = av_zalloc(rc);
        CHECK_RET_TAG_WITH_GOTO(ives, err);
        for (i = 0; i < sample_cnt; i++) {
            priv->encrypt_samples[i].iv = ives + (i * priv->per_sample_iv_size);
            rc = stream_read(s, priv->encrypt_samples[i].iv, priv->per_sample_iv_size);
            CHECK_RET_TAG_WITH_GOTO(rc == priv->per_sample_iv_size, err);
        }
    }

    return 0;
err:
    av_freep((char **)&ives);
    av_freep((char **)&priv->encrypt_samples);
    LOGE(TAG, "senc init iv failed");
    return -1;
}

static int _mp4_read_schm(demux_cls_t *o, mp4_atom_t *atom)
{
    int rc;
    size_t olen;
    char skey[32 + 1]     = {0};
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    int schm_type, schm_version;

    stream_r8(s);    /* version */
    stream_r24be(s); /* flags */

    schm_type = stream_r32le(s);
    if (schm_type != ATOMID('c', 'e', 'n', 'c')) {
        LOGE(TAG, "unsupport encryption type");
        return -1;
    }
    priv->cenc_encrypted = 1;
    /* url example: mem://xx&acodec=mp3&decryption_key=6abed02448b8ffd2224ff54619935526 */
    rc = url_get_item_value(s->url, "decryption_key", skey, sizeof(skey));
    if (rc == 0) {
        if (strlen(skey) != 32) {
            LOGE(TAG, "decryption_key len wrong!");
            return -1;
        }
        rc = bytes_from_hex(skey, priv->cenc_key, sizeof(priv->cenc_key));
    } else if (s->get_dec_cb) {
        olen = sizeof(priv->cenc_key);
        rc   = s->get_dec_cb(priv->major_brand, 4, priv->cenc_key, &olen);
    }

    if (rc < 0) {
        LOGE(TAG, "get decryption_key failed!");
        return -1;
    }

    schm_version = stream_r32be(s);
    if (schm_version != 0x10000) {
        LOGE(TAG, "unsupport encryption algorithm");
        return -1;
    }

    return 0;
}

static const mp4_parser_t g_mp4_parser_table[] = {
    {ATOMID('e', 's', 'd', 's'), _mp4_read_esds}, //1
    {ATOMID('d', 'i', 'n', 'f'), _mp4_read_atom}, //1
    {ATOMID('f', 't', 'y', 'p'), _mp4_read_ftyp}, //1
    {ATOMID('h', 'd', 'l', 'r'), _mp4_read_hdlr}, //1
    {ATOMID('m', 'd', 'a', 't'), _mp4_read_mdat}, //1
    {ATOMID('m', 'd', 'h', 'd'), _mp4_read_mdhd}, //1
    {ATOMID('m', 'd', 'i', 'a'), _mp4_read_atom}, //1
    {ATOMID('m', 'i', 'n', 'f'), _mp4_read_atom}, //1
    {ATOMID('m', 'o', 'o', 'v'), _mp4_read_moov}, //1
    {ATOMID('m', 'v', 'h', 'd'), _mp4_read_mvhd}, //1
    {ATOMID('s', 'i', 'n', 'f'), _mp4_read_atom}, //1 cenc
    {ATOMID('f', 'r', 'm', 'a'), _mp4_read_frma}, //1 cenc
    {ATOMID('s', 'c', 'h', 'm'), _mp4_read_schm}, //1
    {ATOMID('s', 'c', 'h', 'i'), _mp4_read_atom}, //1
    {ATOMID('s', 'e', 'n', 'c'), _mp4_read_senc}, //1
    {ATOMID('t', 'e', 'n', 'c'), _mp4_read_tenc}, //1
    {ATOMID('s', 't', 'b', 'l'), _mp4_read_atom}, //1
    {ATOMID('s', 't', 'c', 'o'), _mp4_read_stco}, //1
    {ATOMID('s', 't', 's', 'c'), _mp4_read_stsc}, //1
    {ATOMID('s', 't', 's', 'd'), _mp4_read_stsd}, //1
    {ATOMID('w', 'a', 'v', 'e'), _mp4_read_wave}, //1
    {ATOMID('s', 't', 's', 'z'), _mp4_read_stsz}, //1
    {ATOMID('s', 't', 't', 's'), _mp4_read_stts}, //1
    {ATOMID('t', 'k', 'h', 'd'), _mp4_read_tkhd}, //1
    {ATOMID('t', 'r', 'a', 'k'), _mp4_read_trak}, //1
};

static atom_parser_t  _find_atom_parser(uint32_t atomid)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(g_mp4_parser_table); i++) {
        if (g_mp4_parser_table[i].type == atomid) {
            return g_mp4_parser_table[i].parser;
        }
    }

    return NULL;
}

static int _mp4_read_atom(demux_cls_t *o, mp4_atom_t *patom)
{
    mp4_atom_t atom;
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = o->priv;
    uint32_t count   = 0;
    atom_parser_t  parser;

    while ((count + 8 <= patom->size) && (!stream_is_eof(s))) {
        atom.size = patom->size;
        atom.type = 0;
        if (patom->size >= 8) {
            atom.size = stream_r32be(s);
            atom.type = stream_r32le(s);
            count += 8;
            if (atom.size == 1) {
                atom.size = stream_r64be(s) - 8;
                count += 8;
            }
        }
        if (atom.size == 0) {
            /* the last atom box */
            return 0;
        }
        atom.size -= 8;
        atom.size  = MIN(atom.size, patom->size - count);
        if (atom.size < 0) {
            return -1;
        }

        parser = _find_atom_parser(atom.type);
        //LOGD(TAG, "find parser. type: %s, size : %d", four_2_str(atom.type), atom.size);
        if (!parser) {
            stream_skip(s, atom.size);
        } else {
            int32_t start_pos = stream_tell(s);
            int left, err;

            err = parser(o, &atom);
            if (err < 0) {
                LOGE(TAG, "type: %s, size : %d", four_2_str(atom.type), atom.size);
                return err;
            }

            //FIXME:
            if ((priv->find_moov && priv->find_mdat && priv->indexes &&
                 (!priv->cenc_encrypted || (priv->cenc_encrypted && priv->encrypt_samples))) || priv->not_atrak) {
                return 0;
            }

            left = atom.size + start_pos - stream_tell(s);
            if (left > 0) {
                stream_skip(s, left);
            } else if (left < 0) {
                return -1;
            }
        }

        count += atom.size;
    }

    return 0;
}

static int _demux_mp4_probe(const avprobe_data_t *pd)
{
    int offset;
    uint32_t tag;
    uint8_t *buf = pd->buf;
    int buf_size = pd->buf_size;

    if (buf_size < 16)
        return 0;

    offset = byte_r32be(buf);
    if (offset < 8)
        return 0;

    tag = byte_r32le(buf + 4);
    if (tag != ATOMID('f', 't', 'y', 'p'))
        return 0;

    return AVPROBE_SCORE_MAX;
}

static int _demux_mp4_open(demux_cls_t *o)
{
    int rc;
    int bits = 16;
    mp4_atom_t atom;
    stream_cls_t *s       = o->s;
    struct mp4_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct mp4_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    o->priv = priv;

    rc = stream_get_size(s);
    atom.size = rc <= 0 ? INT32_MAX : rc;
    atom.type = byte_r32le((const uint8_t*)"root");

    if ((rc = _mp4_read_atom(o, &atom)) < 0) {
        LOGE(TAG, "read atom err: %d", rc);
        goto err;
    }

    if (priv->id == AVCODEC_ID_UNKNOWN) {
        LOGE(TAG, "get codec id err");
        goto err;
    }

    o->ash.sf = sf_make_channel(priv->channel) | sf_make_rate(priv->rate) | sf_make_bit(bits) | sf_make_signed(bits > 8);
    if (priv->id == AVCODEC_ID_MP3) {
        struct mp3_hdr_info hinfo;

        if (priv->cenc_encrypted) {
            priv->aes_param = av_zalloc(sizeof(aes_crypt_t));
            CHECK_RET_TAG_WITH_GOTO(priv->aes_param, err);
            mbedtls_aes_init(&priv->aes_param->ctx);
            mbedtls_aes_setkey_enc(&priv->aes_param->ctx, priv->cenc_key, AES_128_BITS);
        }

        rc = _demux_mp4_read_packet(o, &o->fpkt);
        CHECK_RET_TAG_WITH_GOTO(rc > 0, err);

        rc = mp3_hdr_get((const uint8_t*)o->fpkt.data, &hinfo);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        o->ash.sf = hinfo.sf;
    }

    o->ash.id = priv->id;

    return 0;
err:
    _mp4_read_close(o);
    av_free(priv);
    return -1;
}

static int _demux_mp4_close(demux_cls_t *o)
{
    struct mp4_priv *priv = o->priv;

    _mp4_read_close(o);

    av_free(priv);
    o->priv = NULL;

    return 0;
}

static int _demux_mp4_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int ret;
    uint8_t *iv;
    index_entry_t *e;
    stream_cls_t *s = o->s;
    struct mp4_priv *priv = o->priv;

    if (priv->cur_sample >= priv->nb_indexes) {
        return 0;
    }

#if CONFIG_AV_MP4_IDX_OPT
    if (priv->cur_sample > priv->seg_idxes[priv->cur_seg_idx].end) {
        priv->cur_seg_idx++;
        ret = _mp4_create_indexes(o, priv->seg_idxes[priv->cur_seg_idx].start);
        if (ret < 0) {
            LOGE(TAG, "mp4 re-create indexes fail");
            return -1;
        }
    }
    e = &priv->indexes[priv->cur_sample % IDX_NB_PER_SEGMENT];
#else
    e = &priv->indexes[priv->cur_sample];
#endif
    ret = stream_seek(s, e->pos, SEEK_SET);
    CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

    if (pkt->size < e->size) {
        ret = avpacket_grow(pkt, e->size);
        CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
    }

    ret = stream_read(s, pkt->data, e->size);
    CHECK_RET_TAG_WITH_GOTO(ret == e->size, err);
    pkt->len = e->size;
    pkt->pts = e->timestamp;
    if (priv->cenc_encrypted && priv->per_sample_iv_size) {
        iv = priv->encrypt_samples[priv->cur_sample].iv;
        if (!iv) {
            LOGE(TAG, "iv is null when cenc encrypt!");
            goto err;
        }

        priv->aes_param->offset = 0;
        memset(priv->aes_param->iv, 0, AES_128_BYTES);
        memcpy(priv->aes_param->iv, iv, priv->per_sample_iv_size);
        mbedtls_aes_crypt_ctr(&priv->aes_param->ctx, e->size, &priv->aes_param->offset,
                              priv->aes_param->iv, priv->aes_param->block, pkt->data,
                              pkt->data);
    }
    priv->cur_sample++;

    return pkt->len;
err:
    return -1;
}

#if CONFIG_AV_MP4_IDX_OPT
static int _demux_mp4_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    int x, y, m = 0;
    int i, seg_idx = -1;
    struct mp4_priv *priv = o->priv;

    if (priv->seg_idxes[0].start_ts <= timestamp
        && priv->seg_idxes[priv->nb_seg_idxes - 1].end_ts >= timestamp) {
        for (i = 0; i < priv->nb_seg_idxes; i++) {
            if (priv->seg_idxes[i].start_ts <= timestamp && priv->seg_idxes[i].end_ts >= timestamp) {
                seg_idx = i;
                break;
            }
        }
    }

    if (seg_idx >= 0) {
        if (seg_idx != priv->cur_seg_idx) {
            priv->cur_seg_idx = seg_idx;
            rc = _mp4_create_indexes(o, priv->seg_idxes[priv->cur_seg_idx].start);
            if (rc < 0) {
                LOGE(TAG, "mp4 re-create indexes fail");
                return -1;
            }
        }
    } else {
        return -1;
    }

    x = -1;
    y = priv->seg_idxes[priv->cur_seg_idx].end - priv->seg_idxes[priv->cur_seg_idx].start + 1;
    while (y - x > 1) {
        m = (x + y) >> 1;
        if (priv->indexes[m].timestamp >= timestamp) {
            y = m;
        } else {
            x = m;
        }
    }
    priv->cur_sample = m + priv->seg_idxes[priv->cur_seg_idx].start;

    return 0;
}
#else
static int _demux_mp4_seek(demux_cls_t *o, uint64_t timestamp)
{
    int rc = -1;
    int x, y, m = 0;
    struct mp4_priv *priv = o->priv;

    if (priv->indexes[0].timestamp <= timestamp
        && priv->indexes[priv->nb_indexes - 1].timestamp >= timestamp) {
        x = -1;
        y = priv->nb_indexes;
        while (y - x > 1) {
            m = (x + y) >> 1;
            if (priv->indexes[m].timestamp >= timestamp) {
                y = m;
            } else {
                x = m;
            }
        }
        priv->cur_sample = m;
        rc = 0;
    }

    return rc;
}
#endif

static int _demux_mp4_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct demux_ops demux_ops_mp4 = {
    .name            = "mp4",
    .type            = AVFORMAT_TYPE_MP4,
    .extensions      = { "m4a", "mp4", "mov", NULL },

    .read_probe      = _demux_mp4_probe,

    .open            = _demux_mp4_open,
    .close           = _demux_mp4_close,
    .read_packet     = _demux_mp4_read_packet,
    .seek            = _demux_mp4_seek,
    .control         = _demux_mp4_control,
};
#endif

