/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_ATEMPOER_IPC) && CONFIG_ATEMPOER_IPC

#include "atempo/atempo_cls.h"
#include "atempo_icore.h"

#define TAG                   "atempo_ipc"

struct atempo_ipc_priv {
    atempo_icore_t                 *hdl;
};

static int _atempo_ipc_init(atempo_t *atempo)
{
    int rc;
    atempo_icore_t *hdl = NULL;
    struct atempo_ipc_priv *priv = NULL;

    rc = atempo_icore_init();
    CHECK_RET_TAG_WITH_RET(rc == 0, -1);

    priv = av_zalloc(sizeof(struct atempo_ipc_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    hdl = atempo_icore_new(atempo->rate);
    CHECK_RET_TAG_WITH_GOTO(hdl, err);

    priv->hdl    = hdl;
    atempo->priv = priv;
    return 0;
err:
    av_free(priv);
    return -1;
}

static int _atempo_ipc_write(atempo_t *atempo, const int16_t *in, size_t nb_samples)
{
    int rc = -1;
    struct atempo_ipc_priv *priv = atempo->priv;

    rc = atempo_icore_write(priv->hdl, in, nb_samples);

    return rc;
}

static int _atempo_ipc_read(atempo_t *atempo, int16_t *out, size_t nb_samples)
{
    int rc = -1;
    struct atempo_ipc_priv *priv = atempo->priv;

    rc = atempo_icore_read(priv->hdl, out, nb_samples);

    return rc;
}

static int _atempo_ipc_set_speed(atempo_t *atempo, float speed)
{
    int rc = -1;
    struct atempo_ipc_priv *priv = atempo->priv;

    rc = atempo_icore_set_speed(priv->hdl, speed);

    return rc;
}

static int _atempo_ipc_flush(atempo_t *atempo)
{
    int rc = -1;
    struct atempo_ipc_priv *priv = atempo->priv;

    rc = atempo_icore_flush(priv->hdl);

    return rc;
}

static int _atempo_ipc_uninit(atempo_t *atempo)
{
    struct atempo_ipc_priv *priv = atempo->priv;

    atempo_icore_free(priv->hdl);
    av_free(priv);
    atempo->priv = NULL;
    return 0;
}

const struct atempo_ops atempo_ops_ipc = {
    .name            = "atempo_ipc",

    .init            = _atempo_ipc_init,
    .write           = _atempo_ipc_write,
    .read            = _atempo_ipc_read,
    .set_speed       = _atempo_ipc_set_speed,
    .flush           = _atempo_ipc_flush,
    .uninit          = _atempo_ipc_uninit,
};

#endif

