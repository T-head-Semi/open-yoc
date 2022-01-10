/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_IPC) && CONFIG_DECODER_IPC

#include "avutil/common.h"
#include "avcodec/ad_cls.h"
#include "icore/ad_icore_internal.h"
#include "ad_icore.h"

#define TAG                    "ad_ipc"

struct ad_ipc_priv {
    ad_icore_t             *hdl;
};

static int _ad_ipc_open(ad_cls_t *o)
{
    int rc;
    struct ad_ipc_priv *priv = NULL;
    ad_icore_t *hdl;
    adi_conf_t adi_cnf;

    priv = av_zalloc(sizeof(struct ad_ipc_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    rc = ad_icore_init();
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rc = ad_icore_conf_init(&adi_cnf);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    adi_cnf.sf             = o->ash.sf;
    adi_cnf.block_align    = o->ash.block_align;
    adi_cnf.bps            = o->ash.bps;
    adi_cnf.extradata      = o->ash.extradata;
    adi_cnf.extradata_size = o->ash.extradata_size;

    hdl = ad_icore_open(o->ash.id, &adi_cnf);
    CHECK_RET_TAG_WITH_GOTO(hdl, err);
    ad_icore_get_sf(hdl, &o->ash.sf);

    priv->hdl = hdl;
    o->priv   = priv;

    return 0;
err:
    av_free(priv);

    return -1;
}

static int _ad_ipc_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    int ret = -1;
    struct ad_ipc_priv *priv = o->priv;

    ret = ad_icore_decode(priv->hdl, frame, got_frame, pkt);
    if (*got_frame) {
        o->ash.sf = frame->sf;
    }

    return ret;
}

static int _ad_ipc_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return 0;
}

static int _ad_ipc_reset(ad_cls_t *o)
{
    int rc;
    struct ad_ipc_priv *priv = o->priv;

    rc = ad_icore_reset(priv->hdl);

    return rc;
}

static int _ad_ipc_close(ad_cls_t *o)
{
    struct ad_ipc_priv *priv = o->priv;

    ad_icore_close(priv->hdl);

    av_free(priv);
    o->priv = NULL;
    return 0;
}

const struct ad_ops ad_ops_ipc = {
    .name           = "ipc",
    .id             = AVCODEC_ID_AAC | AVCODEC_ID_MP3,

    .open           = _ad_ipc_open,
    .decode         = _ad_ipc_decode,
    .control        = _ad_ipc_control,
    .reset          = _ad_ipc_reset,
    .close          = _ad_ipc_close,
};

#endif

