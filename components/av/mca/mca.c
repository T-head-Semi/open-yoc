/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "mca/cmca.h"

#define TAG                  "cmca"

static struct mcax_ops       *_mcax_ops;

/**
 * @brief  regist mca ops
 * @param  [in] ops
 * @return 0/-1
 */
int mcax_ops_register(const struct mcax_ops *ops)
{
    CHECK_PARAM(ops, -1);
    if (_mcax_ops) {
        LOGE(TAG, "error. ops had regist yet!, name = %s", _mcax_ops->name);
        return -1;
    }
    _mcax_ops = (struct mcax_ops*)ops;

    return 0;
}

/**
 * @brief  alloc a mca
 * @param  [in] type : MCA_TYPE_XXX
 * @return NULL on error
 */
mcax_t* mcax_new(int type)
{
    int rc;
    mcax_t *mca = NULL;

    CHECK_PARAM(type, NULL);
    if (!_mcax_ops) {
        LOGE(TAG, "error. ops has not regist yet!s");
        return NULL;
    }

    mca = av_zalloc(sizeof(mcax_t));
    CHECK_RET_TAG_WITH_GOTO(mca, err);

    rc = _mcax_ops->init(mca, type);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    mca->ops  = _mcax_ops;
    mca->type = type;
    aos_mutex_new(&mca->lock);

    return mca;
err:
    av_free(mca);
    return NULL;
}

/**
 * Configures IIR filter with 32-bit input/ouput and 32-bit coefficients.
 *
 * Note:
 *   # coeff is 32-bit (Q1.7.24), with size of 5.
 *   # Input/output are 32-bit (Q1.31.0).
 * @return -1/0
 */
int mcax_iir_fxp32_coeff32_config(mcax_t *mca, const fxp32_t *coeff)
{
    int rc;

    CHECK_PARAM(mca && coeff, -1);
    aos_mutex_lock(&mca->lock, AOS_WAIT_FOREVER);
    rc = mca->ops->iir_fxp32_coeff32_config(mca, coeff);
    aos_mutex_unlock(&mca->lock);

    return rc;
}

/**
 * IIR filter with 32-bit input/output.
 *
 * Note:
 *   # input is 32-bit (Q1.31.0), output also 32-bit but its precision is
 *     determined by configuration.
 *   # input_size must be greater than order, and size of output is
 *     (input_size - 2).
 *   # yn1 and yn2 are the 1st and 2nd samples from the last output.
 * @return -1/0
 */
int mcax_iir_fxp32(mcax_t *mca, const fxp32_t *input, size_t input_size, fxp32_t yn1, fxp32_t yn2,
                   fxp32_t *output)
{
    int rc;

    CHECK_PARAM(mca && input && input_size && output, -1);
    aos_mutex_lock(&mca->lock, AOS_WAIT_FOREVER);
    rc = mca->ops->iir_fxp32(mca, input, input_size, yn1, yn2, output);
    aos_mutex_unlock(&mca->lock);

    return rc;
}

/**
 * @brief  free the mca
 * @param  [in] mca
 * @return 0/-1
 */
int mcax_free(mcax_t *mca)
{
    CHECK_PARAM(mca, -1);
    mca->ops->uninit(mca);
    aos_mutex_free(&mca->lock);
    av_free(mca);

    return 0;
}

