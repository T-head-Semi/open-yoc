/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_FLAC) && CONFIG_DECODER_FLAC

#include "avutil/common.h"
#include "avutil/byte_rw.h"
#include "avutil/bio.h"
#include "avformat/flac_rw.h"
#include "avcodec/ad_cls.h"
#include "FLAC/stream_decoder.h"

#define TAG                    "ad_flac"

#define DEC_IBUF_SIZE_MIN      (FLAC_FRAME_SIZE_DEFAULT)

#define OUTPUT_24_16_ENABLE

struct ad_flac_priv {
    FLAC__StreamDecoder        *dec;
    struct flac_stream_info    si;
    uint8_t                    si_find;       ///< may be not have stream info

    uint8_t                    *ibuf;
    size_t                     ipos;          ///< pos of read or write
    size_t                     isize;         ///< valid data size
    size_t                     icap;          ///< cap size

    FLAC__FrameHeader          fhdr;
    FLAC__int32                *opbuf[2];     ///< output pointer: 2 channels max support.
};

static FLAC__StreamDecoderReadStatus _read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    UNUSED(decoder);
    int rc = *bytes, remain;
    struct ad_flac_priv *priv = client_data;

    remain = priv->isize - priv->ipos;
    rc = rc > remain ? remain : rc;
    if (rc > 0) {
        memcpy(buffer, priv->ibuf + priv->ipos, rc);
        priv->ipos += rc;
        remain     -= rc;
        if (remain > 0) {
            memmove(priv->ibuf, priv->ibuf + priv->ipos, remain);
        }
        priv->isize = remain;
        priv->ipos  = 0;
    }
    *bytes = rc;

    return (rc == 0 ? FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM : FLAC__STREAM_DECODER_READ_STATUS_CONTINUE);
}

static FLAC__StreamDecoderWriteStatus _write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
        const FLAC__int32 * const buffer[], void *client_data)
{
    //FIXME:
    UNUSED(decoder);
    struct ad_flac_priv *priv   = client_data;

    priv->fhdr = frame->header;
    if (!(frame->header.blocksize && frame->header.channels && (frame->header.channels <= 2) && frame->header.bits_per_sample)) {
        LOGE(TAG, "write callback err: blocksize = %u, channels = %u, bits = %u", frame->header.blocksize,
             frame->header.channels, frame->header.bits_per_sample);
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    memcpy(priv->opbuf, buffer, sizeof(const FLAC__int32 * const) * frame->header.channels);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void _metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    UNUSED(decoder);
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        struct ad_flac_priv *priv   = client_data;
        struct flac_stream_info *si = &priv->si;

        si->nb_samples = metadata->data.stream_info.total_samples;
        si->rate       = metadata->data.stream_info.sample_rate;
        si->channels   = metadata->data.stream_info.channels;
        si->bits       = metadata->data.stream_info.bits_per_sample;
    }
}

static void _error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    UNUSED(decoder);
    UNUSED(client_data);

    LOGE(TAG, "Got error callback: %s", FLAC__StreamDecoderErrorStatusString[status]);
}

static int _flac_open(struct ad_flac_priv *priv, uint8_t *extradata, size_t extradata_size)
{
    int rc;
    FLAC__StreamDecoderInitStatus init_status;
    uint8_t *ibuf               = NULL;
    FLAC__StreamDecoder *dec    = NULL;
    struct flac_stream_info *si = &priv->si;

    dec = FLAC__stream_decoder_new();
    CHECK_RET_TAG_WITH_RET(dec, -1);

    init_status = FLAC__stream_decoder_init_stream(
                      dec,
                      _read_callback,
                      NULL /* seek_callback */,
                      NULL /* tell_callback */,
                      NULL /* length_callback */,
                      NULL /* eof_callback */,
                      _write_callback,
                      _metadata_callback,
                      _error_callback,
                      (void *)priv);
    CHECK_RET_TAG_WITH_GOTO(init_status == FLAC__STREAM_DECODER_INIT_STATUS_OK, err);

    if (extradata_size && extradata && !priv->si_find) {
        rc = flac_parse_si((const uint8_t*)extradata, si);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        si->fsize_min = si->fsize_min ? si->fsize_min : FLAC_FRAME_SIZE_DEFAULT;
        si->fsize_max = si->fsize_max ? si->fsize_max : FLAC_FRAME_SIZE_DEFAULT;
        priv->si_find = 1;
    }

    //FIXME:
    if (!priv->ibuf) {
        size_t isize = si->fsize_max * 2;

        isize = isize > DEC_IBUF_SIZE_MIN ? isize : DEC_IBUF_SIZE_MIN;
        ibuf  = av_zalloc(isize);
        CHECK_RET_TAG_WITH_GOTO(ibuf, err);

        priv->ibuf = ibuf;
        priv->icap = isize;
    }

    priv->dec = dec;
    return 0;
err:
    av_free(ibuf);
    FLAC__stream_decoder_delete(dec);
    return -1;
}

static int _ad_flac_open(ad_cls_t *o)
{
    int rc;
    struct ad_flac_priv *priv;

    priv = av_zalloc(sizeof(struct ad_flac_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    rc = _flac_open(priv, o->ash.extradata, o->ash.extradata_size);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    if (priv->si_find) {
        struct flac_stream_info *si = &priv->si;

#ifdef OUTPUT_24_16_ENABLE
        int bits = si->bits == 24 ? 16 : si->bits;
        o->ash.sf = sf_make_channel(si->channels) | sf_make_rate(si->rate) | sf_make_bit(bits) | sf_make_signed(1);
#else
        o->ash.sf = sf_make_channel(si->channels) | sf_make_rate(si->rate) | sf_make_bit(si->bits) | sf_make_signed(1);
#endif
    }
    o->priv = priv;

    return 0;
err:
    av_free(priv);
    return -1;
}

static int _ad_flac_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    sf_t sf;
    int ret = -1, i, bits;
    struct ad_flac_priv *priv = o->priv;
    FLAC__StreamDecoder *dec  = priv->dec;
    FLAC__FrameHeader *fhdr   = &priv->fhdr;

    if (pkt->len > priv->icap - priv->isize) {
        LOGE(TAG, "pkt len is too large for flac, %u", pkt->len);
        goto quit;
    }

    memcpy(priv->ibuf + priv->isize, pkt->data, pkt->len);
    priv->isize += pkt->len;

    memset(priv->opbuf, 0, sizeof(priv->opbuf));
    memset(fhdr, 0, sizeof(FLAC__FrameHeader));
    if (!FLAC__stream_decoder_process_single(dec)) {
        LOGE(TAG, "decodeOneFrame: process_single failed");
        goto quit;
    }

    bits = fhdr->bits_per_sample;
#ifdef OUTPUT_24_16_ENABLE
    bits = bits == 24 ? 16 : bits;
#endif
    sf = sf_make_channel(fhdr->channels) | sf_make_rate(fhdr->sample_rate) | sf_make_bit(bits) | sf_make_signed(1);
    if (!((!o->ash.sf || o->ash.sf == sf) && sf && priv->opbuf[0])) {
        LOGE(TAG, "decode  failed, %u, %u, 0x%x", o->ash.sf, sf, priv->opbuf[0]);
        goto quit;
    }

    o->ash.sf         = sf;
    frame->sf         = sf;
    frame->nb_samples = fhdr->blocksize;

    ret = avframe_get_buffer(frame);
    if (ret < 0) {
        LOGD(TAG, "avframe_get_buffer failed, may be oom. sf = %u, ch = %d", sf, fhdr->blocksize);
        goto quit;
    }

    /* write decoded PCM samples */
    if (fhdr->channels == 1 && fhdr->bits_per_sample == 8) {
        int8_t *d = (int8_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            *d++ = priv->opbuf[0][i];
        }
    } else if (fhdr->channels == 2 && fhdr->bits_per_sample == 8) {
        int8_t *d = (int8_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            *d++ = priv->opbuf[0][i];
            *d++ = priv->opbuf[1][i];
        }
    } else if (fhdr->channels == 1 && fhdr->bits_per_sample == 16) {
        int16_t *d = (int16_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            *d++ = priv->opbuf[0][i];
        }
    } else if (fhdr->channels == 2 && fhdr->bits_per_sample == 16) {
        int16_t *d = (int16_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            *d++ = priv->opbuf[0][i];
            *d++ = priv->opbuf[1][i];
        }
#ifdef OUTPUT_24_16_ENABLE
    } else if (fhdr->channels == 1 && fhdr->bits_per_sample == 24) {
        int16_t *d = (int16_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            *d++ = priv->opbuf[0][i] >> 8;
        }
    } else if (fhdr->channels == 2 && fhdr->bits_per_sample == 24) {
        int16_t *d = (int16_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            *d++ = priv->opbuf[0][i] >> 8;
            *d++ = priv->opbuf[1][i] >> 8;
        }
    }
#else
    } else if (fhdr->channels == 1 && fhdr->bits_per_sample == 24)
    {
        int8_t *d = (int8_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            memcpy(d, &priv->opbuf[0][i], 3);
            d += 3;
        }
    } else if (fhdr->channels == 2 && fhdr->bits_per_sample == 24)
    {
        int8_t *d = (int8_t*)frame->data[0];
        for(i = 0; i < fhdr->blocksize; i++) {
            memcpy(d, &priv->opbuf[0][i], 3);
            d += 3;
            memcpy(d, &priv->opbuf[1][i], 3);
            d += 3;
        }
    }
#endif
    *got_frame = 1;
    ret = pkt->len;

quit:
    return ret;
}

static int _ad_flac_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_flac_reset(ad_cls_t *o)
{
    int rc;
    struct ad_flac_priv *priv = o->priv;
    FLAC__StreamDecoder* dec  = priv->dec;

    FLAC__stream_decoder_delete(dec);
    priv->dec   = NULL;
    priv->isize = 0;
    priv->ipos  = 0;
    rc = _flac_open(priv, o->ash.extradata, o->ash.extradata_size);

    return rc;
}

static int _ad_flac_close(ad_cls_t *o)
{
    struct ad_flac_priv *priv = o->priv;
    FLAC__StreamDecoder* dec  = priv->dec;

    FLAC__stream_decoder_delete(dec);

    av_free(priv);
    av_free(priv->ibuf);
    o->priv = NULL;
    return 0;
}

const struct ad_ops ad_ops_flac = {
    .name           = "flac",
    .id             = AVCODEC_ID_FLAC,

    .open           = _ad_flac_open,
    .decode         = _ad_flac_decode,
    .control        = _ad_flac_control,
    .reset          = _ad_flac_reset,
    .close          = _ad_flac_close,
};

#endif

