/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_MCAXER_LOCAL) && CONFIG_MCAXER_LOCAL
#include "avutil/common.h"
#include "avutil/av_typedef.h"
#include "mca/mca_cls.h"
#include "mca/csky_mca.h"

#define TAG                   "mca_local"
static struct {
    fxp32_t                   coeff[5];
    fxp32_t                   input[POST_PROC_SAMPLES_MAX];
    fxp32_t                   output[POST_PROC_SAMPLES_MAX - 2];
} _res_iir  __attribute__((section(".fast_heap")));

static struct {
    aos_mutex_t               iir_lock;
    int                       init;
} g_mcalocal;

#define mca_iir_lock()   (aos_mutex_lock(&g_mcalocal.iir_lock, AOS_WAIT_FOREVER))
#define mca_iir_unlock() (aos_mutex_unlock(&g_mcalocal.iir_lock))

struct mca_local_priv {
    void                      *hdl;
};

static int _mca_local_init(mcax_t *mca, int32_t type)
{
    void *hdl = NULL;
    struct mca_local_priv *priv = NULL;

    if (!g_mcalocal.init) {
        aos_mutex_new(&g_mcalocal.iir_lock);
        g_mcalocal.init = 1;
    }

    priv = av_zalloc(sizeof(struct mca_local_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    priv->hdl = hdl;
    mca->priv = priv;
    return 0;
}

static int _mca_local_iir_fxp32_coeff32_config(mcax_t *mca, const fxp32_t *coeff)
{
    UNUSED(mca);
    mca_iir_lock();
    memcpy(_res_iir.coeff, coeff, sizeof(_res_iir.coeff));
    csky_mca_iir_fxp32_coeff32_config(_res_iir.coeff);
    mca_iir_unlock();

    return 0;
}

static int _mca_local_iir_fxp32(mcax_t *mca, const fxp32_t *input, size_t input_size,
                                fxp32_t yn1, fxp32_t yn2, fxp32_t *output)
{
    UNUSED(mca);
    CHECK_PARAM(input_size <= POST_PROC_SAMPLES_MAX, -1);

    mca_iir_lock();
    memcpy(_res_iir.input, input, input_size * sizeof(fxp32_t));
    csky_mca_iir_fxp32(_res_iir.input, input_size, yn1, yn2, _res_iir.output);
    memcpy(output, _res_iir.output, (input_size - 2) * sizeof(fxp32_t));
    mca_iir_unlock();

    return 0;
}

static int _mca_local_uninit(mcax_t *mca)
{
    struct mca_local_priv *priv = mca->priv;

    av_free(priv);
    mca->priv = NULL;
    return 0;
}

const struct mcax_ops mcax_ops_local = {
    .name                             = "mca_local",

    .init                             = _mca_local_init,
    .iir_fxp32_coeff32_config         = _mca_local_iir_fxp32_coeff32_config,
    .iir_fxp32                        = _mca_local_iir_fxp32,
    .uninit                           = _mca_local_uninit,
};
#endif

