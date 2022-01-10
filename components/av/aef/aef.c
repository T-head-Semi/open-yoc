/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "aef/aef.h"

#define TAG                   "aef"

static struct aefx_ops       *_aefx_ops;

/**
 * @brief  regist aef ops
 * @param  [in] ops
 * @return 0/-1
 */
int aefx_ops_register(const struct aefx_ops *ops)
{
    CHECK_PARAM(ops, -1);
    if (_aefx_ops) {
        LOGE(TAG, "error. ops had regist yet!, name = %s", _aefx_ops->name);
        return -1;
    }
    _aefx_ops = (struct aefx_ops*)ops;
    LOGD(TAG, "regist affecter, name = %s", _aefx_ops->name);

    return 0;
}

/**
 * @brief  alloc a audio effecter
 * @param  [in] rate      : audio freq
 * @param  [in] conf
 * @param  [in] conf_size
 * @param  [in] nsamples_max : max frame len
 * @return NULL on error
 */
aefx_t* aefx_new(uint32_t rate, uint8_t *conf, size_t conf_size, size_t nsamples_max)
{
    int rc;
    aefx_t *aef;

    CHECK_PARAM(rate && nsamples_max, NULL);
    if (!_aefx_ops) {
        LOGE(TAG, "error. ops has not regist yet!s");
        return NULL;
    }

    aef = av_zalloc(sizeof(aefx_t));
    CHECK_RET_TAG_WITH_RET(aef, NULL);
    if (conf && conf_size) {
        aef->conf = av_malloc(conf_size);
        CHECK_RET_TAG_WITH_GOTO(aef->conf, err);
        memcpy(aef->conf, conf, conf_size);
        aef->conf_size = conf_size;
    }
    aef->rate      = rate;
    aef->nsamples_max = nsamples_max;

    rc = _aefx_ops->init(aef);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    aef->ops = _aefx_ops;
    aos_mutex_new(&aef->lock);

    return aef;
err:
    if (aef) {
        av_free(aef->conf);
        av_free(aef);
    }
    return NULL;
}

/**
 * @brief  process samples
 * @param  [in] aef
 * @param  [in] in
 * @param  [in] out
 * @param  [in] nb_samples : samples per channel
 * @return 0/-1
 */
int aefx_process(aefx_t *aef, const int16_t *in, int16_t *out, size_t nb_samples)
{
    int rc;
    size_t n, offset = 0;

    CHECK_PARAM(aef && in && out && nb_samples, -1);
    aos_mutex_lock(&aef->lock, AOS_WAIT_FOREVER);
    n = nb_samples <= aef->nsamples_max ? nb_samples : aef->nsamples_max;
    while (n) {
        rc = aef->ops->process(aef, in + offset, out + offset, n);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        offset += n;
        n       = nb_samples - offset;
        n       = n <= aef->nsamples_max ? n : aef->nsamples_max;
    }
err:
    aos_mutex_unlock(&aef->lock);

    return rc;
}

/**
 * @brief  free the audio effecter
 * @param  [in] aef
 * @return 0/-1
 */
int aefx_free(aefx_t *aef)
{
    CHECK_PARAM(aef, -1);
    aef->ops->uninit(aef);
    aos_mutex_free(&aef->lock);
    av_free(aef->conf);
    av_free(aef);

    return 0;
}


