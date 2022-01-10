/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AVPARSER_ADTS) && CONFIG_AVPARSER_ADTS
#include "avutil/mem_block.h"
#include "avformat/avformat_utils.h"
#include "avformat/adts_rw.h"
#include "avformat/avparser_cls.h"

#define TAG                    "avparser_adts"

struct adts_psr_priv {
    mblock_t                   *mb;
    uint8_t                    with_asc;     ///< Audio Specific Config
    m4a_cnf_t                  m4ac;
};

static int _avparser_adts_open(avparser_t *psr, uint8_t *extradata, size_t extradata_size)
{
    int rc;
    mblock_t *mb = NULL;
    struct adts_psr_priv *priv;

    priv = av_zalloc(sizeof(struct adts_psr_priv));

    if (extradata_size && extradata) {
        uint32_t value;

        value = byte_r16be(extradata);
        if ((value & 0xfff0) != 0xfff0) {
            rc = m4a_decode_asc(&priv->m4ac, extradata, extradata_size);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

            mb = mblock_new(256, 0);
            CHECK_RET_TAG_WITH_GOTO(mb, err);

            priv->with_asc = 1;
            priv->mb       = mb;
        }
    }

    psr->priv = priv;
    return 0;
err:
    av_free(priv);
    return -1;
}

static int _avparser_adts_close(avparser_t *psr)
{
    struct adts_psr_priv *priv = psr->priv;

    mblock_free(priv->mb);
    av_free(priv);

    return 0;
}

static int _avparser_adts_parse(avparser_t *psr, const uint8_t *ibuf, size_t isize, size_t *ipos, uint8_t **obuf, size_t *osize)
{
    int rc;
    int size;
    struct adts_psr_priv *priv = psr->priv;
    mblock_t *mb               = priv->mb;
    struct adts_hdr info, *hinfo = &info;

    if (priv->with_asc) {
        size = ADTS_HDR_SIZE + isize;
        rc   = mblock_grow(mb, size);
        CHECK_RET_TAG_WITH_RET(rc == 0, -1);

        rc = adts_hdr_encode(&priv->m4ac, mb->data, isize);
        CHECK_RET_TAG_WITH_RET(rc == 0, -1);
        memcpy((char*)mb->data + ADTS_HDR_SIZE, ibuf, isize);
        *ipos  = isize;
        *osize = size;
        *obuf  = (uint8_t*)mb->data;
    } else {
        rc = adts_hdr_get(ibuf, hinfo);
        if (rc == 0) {
            *ipos  = hinfo->flen;
            *osize = hinfo->flen;
            *obuf  = (uint8_t*)ibuf;
        }
    }

    return rc;
}

const struct avparser_ops avparser_ops_adts = {
    .name            = "adts",
    .id              = AVCODEC_ID_AAC,

    .open            = _avparser_adts_open,
    .close           = _avparser_adts_close,
    .parse           = _avparser_adts_parse,
};
#endif

