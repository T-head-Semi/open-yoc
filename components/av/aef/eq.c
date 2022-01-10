/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/av_typedef.h"
#include "aef/eq.h"

#define TAG                   "eq"

static struct eqx_ops       *_eqx_ops;

/**
 * @brief  regist eq ops
 * @param  [in] ops
 * @return 0/-1
 */
int eqx_ops_register(const struct eqx_ops *ops)
{
    CHECK_PARAM(ops, -1);
    if (_eqx_ops) {
        LOGE(TAG, "error. ops had regist yet!, name = %s", _eqx_ops->name);
        return -1;
    }
    _eqx_ops = (struct eqx_ops*)ops;
    LOGD(TAG, "regist equalizer, name = %s", _eqx_ops->name);

    return 0;
}

/**
 * @brief  alloc a equalizer
 * @param  [in] rate
 * @param  [in] eq_segments : eq filter segment number max
 * @return NULL on error
 */
eqx_t* eqx_new(uint32_t rate, uint8_t eq_segments)
{
    int rc;
    eqx_t *eq       = NULL;
    eqfp_t *params = NULL;

    CHECK_PARAM(rate && eq_segments, NULL);
    if (!_eqx_ops) {
        LOGE(TAG, "error. ops has not regist yet!s");
        return NULL;
    }

    eq     = av_zalloc(sizeof(eqx_t));
    params = av_zalloc(sizeof(eqfp_t) * eq_segments);
    CHECK_RET_TAG_WITH_GOTO(eq && params, err);
    eq->nsamples_max = POST_PROC_SAMPLES_MAX;

    rc = _eqx_ops->init(eq, rate, eq_segments);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    eq->ops         = _eqx_ops;
    eq->rate        = rate;
    eq->params      = params;
    eq->eq_segments = eq_segments;
    aos_mutex_new(&eq->lock);

    return eq;
err:
    av_free(params);
    av_free(eq);
    return NULL;
}

/**
 * @brief  control whole eq filter enable/disable
 * @param  [in] eq
 * @param  [in] enable
 * @return 0/-1
 */
int eqx_set_enable(eqx_t *eq, uint8_t enable)
{
    int rc = 0;

    CHECK_PARAM(eq, -1);
    aos_mutex_lock(&eq->lock, AOS_WAIT_FOREVER);
    if (eq->enable != enable) {
        rc = eq->ops->set_enable(eq, enable);
        eq->enable = (rc == 0) ? enable : eq->enable;
    }
    aos_mutex_unlock(&eq->lock);

    return rc;
}

/**
 * @brief  set config param to the filter segment
 * @param  [in] eq
 * @param  [in] segid : id of the filter segment
 * @param  [in] param
 * @return 0/-1
 */
int eqx_set_param(eqx_t *eq, uint8_t segid, const eqfp_t *param)
{
    int rc;

    CHECK_PARAM(eq && param && (segid < eq->eq_segments), -1);
    if (!(param->gain >= -24.0 && param->gain <= 18.0
          && param->q >= 0.1 && param->q <= 50.0
          && param->type > EQF_TYPE_UNKNOWN && param->type < EQF_TYPE_MAX)) {
        LOGE(TAG, "eq param error");
        return -1;
    }
    aos_mutex_lock(&eq->lock, AOS_WAIT_FOREVER);
    rc = eq->ops->set_param(eq, segid, param);
    if (rc == 0) {
        memcpy(&eq->params[segid], param, sizeof(eqfp_t));
    }
    aos_mutex_unlock(&eq->lock);

    return rc;
}

/**
 * @brief  process samples
 * @param  [in] eq
 * @param  [in] in
 * @param  [in] out
 * @param  [in] nb_samples : samples per channel
 * @return 0/-1
 */
int eqx_process(eqx_t *eq, const int16_t *in, int16_t *out, size_t nb_samples)
{
    int rc;
    size_t n, offset = 0;

    CHECK_PARAM(eq && in && out && nb_samples, -1);
    aos_mutex_lock(&eq->lock, AOS_WAIT_FOREVER);
    n = nb_samples <= eq->nsamples_max ? nb_samples : eq->nsamples_max;
    while (n) {
        rc = eq->ops->process(eq, in + offset, out + offset, n);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        offset += n;
        n       = nb_samples - offset;
        n       = n <= eq->nsamples_max ? n : eq->nsamples_max;
    }
err:
    aos_mutex_unlock(&eq->lock);

    return rc;
}

/**
 * @brief  free the equalizer
 * @param  [in] eq
 * @return 0/-1
 */
int eqx_free(eqx_t *eq)
{
    CHECK_PARAM(eq, -1);
    eq->ops->uninit(eq);
    aos_mutex_free(&eq->lock);
    av_free(eq->params);
    av_free(eq);

    return 0;
}


