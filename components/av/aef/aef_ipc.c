/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AEFXER_IPC) && CONFIG_AEFXER_IPC

#include "aef/aef_cls.h"
#include "aef_icore.h"

#define TAG                   "aef_ipc"

struct aef_ipc_priv {
    aef_icore_t                 *hdl;
};

static int _aef_ipc_init(aefx_t *aef)
{
    int rc;
    aef_icore_t *hdl = NULL;
    struct aef_ipc_priv *priv = NULL;

    rc = aef_icore_init();
    CHECK_RET_TAG_WITH_RET(rc == 0, -1);

    priv = av_zalloc(sizeof(struct aef_ipc_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    hdl = aef_icore_new(aef->rate, aef->conf, aef->conf_size, aef->nsamples_max);
    CHECK_RET_TAG_WITH_GOTO(hdl, err);

    priv->hdl = hdl;
    aef->priv  = priv;
    return 0;
err:
    av_free(priv);
    return -1;
}

static int _aef_ipc_process(aefx_t *aef, const int16_t *in, int16_t *out, size_t nb_samples)
{
    int rc = -1;
    struct aef_ipc_priv *priv = aef->priv;

    rc = aef_icore_process(priv->hdl, in, out, nb_samples);

    return rc;
}

static int _aef_ipc_uninit(aefx_t *aef)
{
    struct aef_ipc_priv *priv = aef->priv;

    aef_icore_free(priv->hdl);
    av_free(priv);
    aef->priv = NULL;
    return 0;
}

const struct aefx_ops aefx_ops_ipc = {
    .name            = "aef_ipc",

    .init            = _aef_ipc_init,
    .process         = _aef_ipc_process,
    .uninit          = _aef_ipc_uninit,
};

#endif

