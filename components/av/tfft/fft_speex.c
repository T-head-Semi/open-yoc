/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_FFTXER_SPEEX) && CONFIG_FFTXER_SPEEX

#include "tfft/fft_cls.h"
#include "speex/fftwrap.h"

#define TAG                   "fft_speex"

struct fft_speex_priv {
    void                      *hdl;
};

static int _fft_speex_init(fftx_t *fft, size_t size)
{
    void *hdl = NULL;
    struct fft_speex_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct fft_speex_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    hdl = spx_fft_init(size);
    CHECK_RET_TAG_WITH_GOTO(hdl, err);

    priv->hdl = hdl;
    fft->priv = priv;
    return 0;
err:
    av_free(priv);
    return -1;
}

static int _fft_speex_forward(fftx_t *fft, const int16_t *in, int16_t *out)
{
    struct fft_speex_priv *priv = fft->priv;

    spx_fft(priv->hdl, (spx_word16_t*)in, (spx_word16_t*)out);

    return 0;
}

static int _fft_speex_backward(fftx_t *fft, const int16_t *in, int16_t *out)
{
    struct fft_speex_priv *priv = fft->priv;

    spx_ifft(priv->hdl, (spx_word16_t*)in, (spx_word16_t*)out);

    return 0;
}

static int _fft_speex_uninit(fftx_t *fft)
{
    struct fft_speex_priv *priv = fft->priv;

    spx_fft_destroy(priv->hdl);
    av_free(priv);
    fft->priv = NULL;
    return 0;
}

const struct fftx_ops fftx_ops_speex = {
    .name            = "fft_speex",

    .init            = _fft_speex_init,
    .forward         = _fft_speex_forward,
    .backward        = _fft_speex_backward,
    .uninit          = _fft_speex_uninit,
};

#endif

