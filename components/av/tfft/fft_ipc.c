/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_FFTXER_IPC) && CONFIG_FFTXER_IPC

#include "tfft/fft_cls.h"
#include "fft_icore.h"

#define TAG                   "fft_ipc"

struct fft_ipc_priv {
    fft_icore_t                *hdl;
};

static int _fft_ipc_init(fftx_t *fft, size_t size)
{
    int rc;
    void *hdl = NULL;
    struct fft_ipc_priv *priv = NULL;

    rc = fft_icore_init();
    CHECK_RET_TAG_WITH_RET(rc == 0, -1);

    priv = av_zalloc(sizeof(struct fft_ipc_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    hdl = fft_icore_new(size);
    CHECK_RET_TAG_WITH_GOTO(hdl, err);

    priv->hdl = hdl;
    fft->priv = priv;
    return 0;
err:
    av_free(priv);
    return -1;
}

static int _fft_ipc_forward(fftx_t *fft, const int16_t *in, int16_t *out)
{
    struct fft_ipc_priv *priv = fft->priv;

    fft_icore_forward(priv->hdl, in, out);

    return 0;
}

static int _fft_ipc_backward(fftx_t *fft, const int16_t *in, int16_t *out)
{
    struct fft_ipc_priv *priv = fft->priv;

    fft_icore_backward(priv->hdl, in, out);

    return 0;
}

static int _fft_ipc_uninit(fftx_t *fft)
{
    struct fft_ipc_priv *priv = fft->priv;

    fft_icore_free(priv->hdl);
    av_free(priv);
    fft->priv = NULL;
    return 0;
}

const struct fftx_ops fftx_ops_ipc = {
    .name            = "fft_ipc",

    .init            = _fft_ipc_init,
    .forward         = _fft_ipc_forward,
    .backward        = _fft_ipc_backward,
    .uninit          = _fft_ipc_uninit,
};

#endif

