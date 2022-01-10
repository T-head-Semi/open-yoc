/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_STREAMER_CRYPTO) && CONFIG_STREAMER_CRYPTO && defined(CONFIG_USING_TLS)
#include "avutil/misc.h"
#include "avutil/url_parse.h"
#include "stream/stream.h"
#include "avutil/socket_rw.h"
#include "mbedtls/aes.h"
#include "mbedtls/debug.h"

#define TAG                    "s_crypto"
#define CRYPTO_BLOCK_NUM       (64)
#define CRYPTO_BLOCK_SIZE      (16)
#define CRYPTO_SIZE_DEFAULT    (CRYPTO_BLOCK_NUM * CRYPTO_BLOCK_SIZE)

struct crypto_priv {
    stream_cls_t               *rs;                         ///< real stream
    mbedtls_aes_context        ctx;
    uint8_t                    eof;
    uint8_t                    iv[16];                      ///< decrypt for the first 16B
    uint8_t                    dec_iv[16];
    uint16_t                   ilen;                        ///< total len of input data, contains decrypted
    uint16_t                   ipos;                        ///< decrypted pos
    uint16_t                   olen;                        ///< total valid len of obuf, contains readed data
    uint16_t                   opos;                        ///< read pos by the user
    uint8_t                    ibuf[CRYPTO_SIZE_DEFAULT];
    uint8_t                    obuf[CRYPTO_SIZE_DEFAULT];
};

// example: crypto://http://www.baidu.com/xx.mp3?key=111&iv=222
static int _stream_crypto_open(stream_cls_t *o, int mode)
{
    uint8_t key[16];
    char s_iv[32 + 1], s_key[32 + 1];
    stm_conf_t stm_cnf;
    stream_cls_t *rs         = NULL;
    struct crypto_priv *priv = NULL;

    UNUSED(mode);
    priv = av_zalloc(sizeof(struct crypto_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    memset(&s_iv, 0, sizeof(s_iv));
    memset(&s_key, 0, sizeof(s_key));
    url_get_item_value(o->url, "key", s_key, sizeof(s_key));
    CHECK_RET_TAG_WITH_GOTO(strlen(s_key), err);
    url_get_item_value(o->url, "iv", s_iv, sizeof(s_iv));
    CHECK_RET_TAG_WITH_GOTO(strlen(s_iv), err);

    bytes_from_hex(s_key, key, sizeof(key));
    bytes_from_hex(s_iv, priv->iv, sizeof(priv->iv));
    memcpy(&priv->dec_iv, &priv->iv, sizeof(priv->iv));

    stream_conf_init(&stm_cnf);
    stm_cnf.rcv_timeout           = o->rcv_timeout;
    stm_cnf.get_dec_cb            = o->get_dec_cb;
    stm_cnf.cache_size            = o->cache_size;
    stm_cnf.cache_start_threshold = o->cache_start_threshold;
    memcpy(&stm_cnf.irq, &o->irq, sizeof(irq_av_t));
    rs = stream_open(o->url + strlen("crypto://"), &stm_cnf);
    CHECK_RET_TAG_WITH_GOTO(rs, err);

    mbedtls_aes_init(&priv->ctx);
    mbedtls_aes_setkey_dec(&priv->ctx, key, sizeof(key) * 8);

    priv->rs = rs;
    o->size  = stream_get_size(rs);
    o->priv  = priv;

    return 0;
err:
    av_free(priv);
    return -1;
}

static int _stream_crypto_close(stream_cls_t *o)
{
    struct crypto_priv *priv = o->priv;

    if (priv->rs) {
        stream_close(priv->rs);
        priv->rs = NULL;
    }

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _stream_crypto_read(stream_cls_t *o, uint8_t *buf, size_t count)
{
    int rc;
    uint16_t remain, nb_blocks;
    struct crypto_priv *priv = o->priv;
    stream_cls_t *rs         = priv->rs;

retry:
    remain = priv->olen - priv->opos;
    if (remain) {
        rc = MIN(count, remain);
        memcpy(buf, &priv->obuf[priv->opos], rc);
        priv->opos += rc;
        return rc;
    } else {
        priv->olen = 0;
        priv->opos = 0;
    }

    while (priv->ilen - priv->ipos < CRYPTO_BLOCK_SIZE) {
        rc = stream_read(rs, &priv->ibuf[priv->ilen], sizeof(priv->ibuf) - priv->ilen);
        if (rc <= 0) {
            priv->eof = 1;
            break;
        }
        priv->ilen += rc;
    }

    nb_blocks = (priv->ilen - priv->ipos) / CRYPTO_BLOCK_SIZE;
    if (!nb_blocks) {
        LOGD(TAG, "may be eof, ilen = %u, opos = %u", priv->ilen, priv->opos);
        return 0;
    }

    rc = mbedtls_aes_crypt_cbc(&priv->ctx, MBEDTLS_AES_DECRYPT, nb_blocks * CRYPTO_BLOCK_SIZE,
                               priv->dec_iv, &priv->ibuf[priv->ipos], &priv->obuf[priv->olen]);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    rc = nb_blocks * CRYPTO_BLOCK_SIZE;
    priv->olen += rc;
    priv->ipos += rc;
    if (priv->ipos >= sizeof(priv->ibuf) / 2) {
        priv->ilen -= rc;
        if (priv->ilen) {
            memmove(priv->ibuf, &priv->ibuf[priv->ipos], priv->ilen);
        }
        priv->ipos = 0;
    }

    goto retry;
err:
    return -1;
}

static int _stream_crypto_write(stream_cls_t *o, const uint8_t *buf, size_t count)
{
    //TODO
    return -1;
}

static int _stream_crypto_seek(stream_cls_t *o, int32_t pos)
{
    int rc = -1, diff;
    uint32_t cpos, nb_blocks;
    uint8_t buf[CRYPTO_BLOCK_SIZE * 2];
    struct crypto_priv *priv = o->priv;
    stream_cls_t *rs         = priv->rs;

    priv->ilen = 0;
    priv->ipos = 0;
    priv->olen = 0;
    priv->opos = 0;
    nb_blocks = pos / CRYPTO_BLOCK_SIZE;
    if (nb_blocks) {
        nb_blocks--;
        cpos = nb_blocks * CRYPTO_BLOCK_SIZE;
    } else {
        cpos = 0;
        memcpy(&priv->dec_iv, &priv->iv, sizeof(priv->iv));
    }

    rc = stream_seek(rs, cpos, SEEK_SET);
    CHECK_RET_TAG_WITH_RET(rc == 0, -1);

    diff = pos - cpos;
    if (diff) {
        while (diff > 0) {
            rc = _stream_crypto_read(o, buf, diff);
            if (rc <= 0) {
                break;
            }
            diff -= rc;
        }
        if (diff) {
            LOGE(TAG, "crypto diff = %d", diff);
            return -1;
        }
    }

    return 0;
}

static int _stream_crypto_control(stream_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO:
    return -1;
}

const struct stream_ops stream_ops_crypto = {
    .name            = "crypto",
    .type            = STREAM_TYPE_CRYPTO,
    .protocols       = { "crypto", NULL },

    .open            = _stream_crypto_open,
    .close           = _stream_crypto_close,
    .read            = _stream_crypto_read,
    .write           = _stream_crypto_write,
    .seek            = _stream_crypto_seek,
    .control         = _stream_crypto_control,
};
#endif

