/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "tfft/fft.h"

#define TAG                  "fft"

static struct fftx_ops       *_fftx_ops;

/**
 * @brief  regist fft ops
 * @param  [in] ops
 * @return 0/-1
 */
int fftx_ops_register(const struct fftx_ops *ops)
{
    CHECK_PARAM(ops, -1);
    if (_fftx_ops) {
        LOGE(TAG, "error. ops had regist yet!, name = %s", _fftx_ops->name);
        return -1;
    }
    _fftx_ops = (struct fftx_ops*)ops;
    LOGD(TAG, "regist fft, name = %s", _fftx_ops->name);

    return 0;
}

/**
 * @brief  alloc a fft
 * @param  [in] size : N
 * @return NULL on error
 */
fftx_t* fftx_new(size_t size)
{
    int rc;
    fftx_t *fft = NULL;

    CHECK_PARAM(size, NULL);
    if (!_fftx_ops) {
        LOGE(TAG, "error. ops has not regist yet!s");
        return NULL;
    }

    fft = av_zalloc(sizeof(fftx_t));
    CHECK_RET_TAG_WITH_GOTO(fft, err);

    rc = _fftx_ops->init(fft, size);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    fft->ops  = _fftx_ops;
    fft->size = size;
    aos_mutex_new(&fft->lock);

    return fft;
err:
    av_free(fft);
    return NULL;
}

/**
 * @brief  fft forward
 * @param  [in] fft
 * @param  [in] in
 * @param  [in] out
 * @return 0/-1
 */
int fftx_forward(fftx_t *fft, const int16_t *in, int16_t *out)
{
    int rc;

    CHECK_PARAM(fft && in && out, -1);
    aos_mutex_lock(&fft->lock, AOS_WAIT_FOREVER);
    rc = fft->ops->forward(fft, in, out);
    aos_mutex_unlock(&fft->lock);

    return rc;
}

/**
 * @brief  fft backward
 * @param  [in] fft
 * @param  [in] in
 * @param  [in] out
 * @return 0/-1
 */
int fftx_backward(fftx_t *fft, const int16_t *in, int16_t *out)
{
    int rc;

    CHECK_PARAM(fft && in && out, -1);
    aos_mutex_lock(&fft->lock, AOS_WAIT_FOREVER);
    rc = fft->ops->backward(fft, in, out);
    aos_mutex_unlock(&fft->lock);

    return rc;
}

/**
 * @brief  free the fft
 * @param  [in] fft
 * @return 0/-1
 */
int fftx_free(fftx_t *fft)
{
    CHECK_PARAM(fft, -1);
    fft->ops->uninit(fft);
    aos_mutex_free(&fft->lock);
    av_free(fft);

    return 0;
}

