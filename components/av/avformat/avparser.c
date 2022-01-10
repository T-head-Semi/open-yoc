/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/misc.h"
#include "avformat/avparser.h"

#define TAG    "avparser"

static struct {
    int                             cnt;
    const struct avparser_ops       *ops[AVPARSER_OPS_MAX];
} g_avparsers;

static const struct avparser_ops* _get_avparser_ops_by_id(avcodec_id_t id)
{
    int i;

    for (i = 0; i < g_avparsers.cnt; i++) {
        if (g_avparsers.ops[i]->id & id) {
            return g_avparsers.ops[i];
        }
    }

    LOGD(TAG, "get avparser ops err, id = %u", id);
    return NULL;
}

/**
 * @brief  regist avparser ops
 * @param  [in] ops
 * @return 0/-1
 */
int avparser_ops_register(const struct avparser_ops *ops)
{
    int i;

    if (ops && (g_avparsers.cnt < AVPARSER_OPS_MAX)) {
        for (i = 0; i < g_avparsers.cnt; i++) {
            if (strcmp(ops->name, g_avparsers.ops[i]->name) == 0) {
                /* replicate  */
                break;
            }
        }

        if (i == g_avparsers.cnt) {
            g_avparsers.ops[g_avparsers.cnt] = ops;
            g_avparsers.cnt++;
        }
        return 0;
    }

    LOGE(TAG, "avparser ops regist fail");
    return -1;
}

/**
 * @brief  open/create one avparser
 * @param  [in] id
 * @param  [in] extradata
 * @param  [in] extradata_size
 * @return NULL on err
 */
avparser_t* avparser_open(avcodec_id_t id, uint8_t *extradata, size_t extradata_size)
{
    int rc;
    avparser_t *psr = NULL;

    CHECK_PARAM(id, NULL);
    psr = av_zalloc(sizeof(avparser_t));
    CHECK_RET_TAG_WITH_GOTO(psr, err);

    psr->ops = _get_avparser_ops_by_id(id);
    if (!psr->ops) {
        //LOGD(TAG, "can't find suitable parser type. id = %d, name = %s", id, get_codec_name(id));
        goto err;
    }
    LOGD(TAG, "find a parser, name = %s, id = %d", psr->ops->name, id);

    rc = psr->ops->open ? psr->ops->open(psr, extradata, extradata_size) : 0;
    CHECK_RET_TAG_WITH_GOTO(rc != -1, err);
    aos_mutex_new(&psr->lock);

    return psr;
err:
    av_free(psr);
    return NULL;
}

/**
 * @brief  parse the in-buf
 * @param  [in] psr
 * @param  [in] ibuf
 * @param  [in] isize
 * @param  [in] ipos  : new pos of ibuf parsed
 * @param  [in] obuf  : out buf after parsed
 * @param  [in] osize
 * @return 0/-1
 */
int avparser_parse(avparser_t *psr, const uint8_t *ibuf, size_t isize, size_t *ipos, uint8_t **obuf, size_t *osize)
{
    int rc;

    CHECK_PARAM(psr && ibuf && isize && obuf && osize && ipos, -1);
    aos_mutex_lock(&psr->lock, AOS_WAIT_FOREVER);
    *ipos  = 0;
    *osize = 0;
    rc = psr->ops->parse(psr, ibuf, isize, ipos, obuf, osize);
    if (rc < 0) {
        LOGE(TAG, "parser fail, rc = %d", rc);
    }
    aos_mutex_unlock(&psr->lock);

    return rc;
}

/**
 * @brief  close/destroy avparser
 * @param  [in] o
 * @return 0/-1
 */
int avparser_close(avparser_t *psr)
{
    int rc = 0;

    CHECK_PARAM(psr, -1);
    if (psr->ops->close)
        rc = psr->ops->close(psr);

    aos_mutex_free(&psr->lock);
    av_free(psr);

    return rc;
}


