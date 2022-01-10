/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_RESAMPLER_SPEEX) && CONFIG_RESAMPLER_SPEEX

#include "swresample/resample_cls.h"
#include "speex/speex_resampler.h"

#define TAG                   "res_srs"

struct res_speex_priv {
    SpeexResamplerState       *srs;
};

static int _resample_speex_init(resx_t *r)
{
    int error = 0;
    SpeexResamplerState* srs = NULL;
    struct res_speex_priv *priv;

    priv = av_zalloc(sizeof(struct res_speex_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    srs = speex_resampler_init((spx_uint32_t)r->channels, (spx_uint32_t)r->irate, (spx_uint32_t)r->orate, 0, &error);
    CHECK_RET_TAG_WITH_GOTO(srs, err);

    priv->srs = srs;
    r->priv   = priv;

    return 0;
err:
    av_free(priv);
    return -1;
}

static int _resample_speex_convert(resx_t *r, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, rc;
    size_t idone = nb_isamples, odone = nb_osamples;
    struct res_speex_priv *priv = r->priv;
    SpeexResamplerState* srs    = priv->srs;

    for (i = 0; i < r->channels; i++) {
        rc = speex_resampler_process_int(srs, (spx_uint32_t)i, (const spx_int16_t *)in[i], (spx_uint32_t *)&idone,
                                         (spx_int16_t *)out[i], (spx_uint32_t *)&odone);
        if (rc != RESAMPLER_ERR_SUCCESS) {
            LOGE(TAG, "srs srsess outsize failed, error = %s, idone = %u, odone= %u, nb_isamples = %u, nb_osamples = %u",
                 rc, idone, odone, nb_isamples, nb_osamples);
            return -1;
        }
    }

    return odone;
}

static int _resample_speex_uninit(resx_t *r)
{
    struct res_speex_priv *priv = r->priv;
    SpeexResamplerState* srs    = priv->srs;

    speex_resampler_destroy(srs);
    av_free(priv);
    r->priv = NULL;
    return 0;
}

const struct resx_ops resx_ops_speex = {
    .name            = "speex",

    .init            = _resample_speex_init,
    .convert         = _resample_speex_convert,
    .uninit          = _resample_speex_uninit,
};

#endif


