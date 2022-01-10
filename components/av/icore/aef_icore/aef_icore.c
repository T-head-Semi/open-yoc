/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AEFXER_IPC) && CONFIG_AEFXER_IPC
#include "ipc.h"
#include <csi_core.h>
#include "avutil/mem_block.h"
#include "aef_icore.h"
#include "icore/aef_icore_internal.h"

#define TAG                     "aef_icore_ap"
#define AEF_MB_SIZE_DEFAULT      (2*1024)

struct aef_icore {
    uint32_t        rate;
    size_t          nsamples_max;

    void            *aef;      // aefx_t
    void            *priv;     // priv only for ap/cpu0
};

struct aef_icore_ap_priv {
    mblock_t        *in_mb;
    mblock_t        *out_mb;
};

static struct {
    ipc_t           *ipc;
    int             init;
} g_aef_icore;

static int _ipc_cmd_send(icore_msg_t *data, int sync)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    msg.service_id  = AEF_ICORE_IPC_SERIVCE_ID;
    msg.command     = IPC_CMD_AEF_ICORE;
    msg.req_data    = data;
    msg.req_len     = ICORE_MSG_SIZE + data->size;
    msg.resp_data   = data;
    msg.resp_len    = ICORE_MSG_SIZE + data->size;
    msg.flag       |= sync;

    ipc_message_send(g_aef_icore.ipc, &msg, AOS_WAIT_FOREVER);

    return data->ret.code;
}

/**
 * @brief  alloc a inter-core audio effecter
 * @param  [in] rate      : audio freq
 * @param  [in] conf
 * @param  [in] conf_size
 * @param  [in] nsamples_max : max frame len
 * @return NULL on error
 */
aef_icore_t* aef_icore_new(uint32_t rate, uint8_t *conf, size_t conf_size, size_t nsamples_max)
{
    int rc;
    aef_icore_t *hdl               = NULL;
    struct aef_icore_ap_priv *priv = NULL;
    mblock_t *in_mb               = NULL, *out_mb = NULL, *conf_mb = NULL;
    icore_msg_t *msg_new          = NULL;
    aef_icore_new_t *inp;

    CHECK_PARAM(rate && nsamples_max, NULL);
    hdl  = av_zalloc(sizeof(aef_icore_t));
    priv = av_zalloc(sizeof(struct aef_icore_ap_priv));
    CHECK_RET_TAG_WITH_GOTO(hdl && priv, err);

    in_mb = mblock_new(AEF_MB_SIZE_DEFAULT, 16);
    out_mb = mblock_new(AEF_MB_SIZE_DEFAULT, 16);
    CHECK_RET_TAG_WITH_GOTO(in_mb && out_mb, err);

    msg_new = icore_msg_new(ICORE_CMD_AEF_NEW, sizeof(aef_icore_new_t));
    CHECK_RET_TAG_WITH_GOTO(msg_new, err);

    inp = icore_get_msg(msg_new, aef_icore_new_t);
    inp->rate      = rate;
    inp->nsamples_max = nsamples_max;
    if (conf && conf_size) {
        conf_mb = mblock_new(conf_size, 16);
        CHECK_RET_TAG_WITH_GOTO(conf_mb, err);
        inp->conf      = conf_mb->data;
        inp->conf_size = conf_size;
        memcpy((void*)inp->conf, (const void*)conf, conf_size);
        csi_dcache_clean_invalid_range((uint32_t*)inp->conf, conf_size);
    }

    rc = _ipc_cmd_send(msg_new, MESSAGE_SYNC);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    priv->in_mb    = in_mb;
    priv->out_mb   = out_mb;
    hdl->aef       = inp->aef;
    hdl->rate      = rate;
    hdl->nsamples_max = nsamples_max;
    hdl->priv      = priv;
    icore_msg_free(msg_new);
    mblock_free(conf_mb);

    return hdl;
err:
    mblock_free(in_mb);
    mblock_free(out_mb);
    mblock_free(conf_mb);
    icore_msg_free(msg_new);
    av_free(hdl);
    av_free(priv);
    return NULL;
}

/**
 * @brief  process samples
 * @param  [in] hdl
 * @param  [in] in
 * @param  [in] out
 * @param  [in] nb_samples : samples per channel
 * @return 0/-1
 */
int aef_icore_process(aef_icore_t *hdl, const int16_t *in, int16_t *out, size_t nb_samples)
{
    int rc = -1;
    size_t size;
    icore_msg_t *msg;
    aef_icore_process_t *inp;
    struct aef_icore_ap_priv *priv;

    CHECK_PARAM(hdl && in && out && nb_samples, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_AEF_PROCESS, sizeof(aef_icore_process_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    size = nb_samples * sizeof(int16_t);
    rc = mblock_grow(priv->in_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    rc = mblock_grow(priv->out_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, aef_icore_process_t);
    inp->aef        = hdl->aef;
    inp->nb_samples = nb_samples;
    inp->in         = priv->in_mb->data;
    inp->out        = priv->out_mb->data;
    memcpy((void*)inp->in, (const void*)in, size);
    csi_dcache_invalid_range((uint32_t*)inp->out, size);
    csi_dcache_clean_invalid_range((uint32_t*)inp->in, size);

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    if (rc == 0) {
        memcpy((void*)out, (const void*)inp->out, size);
    }
    icore_msg_free(msg);

    return rc;
err:
    icore_msg_free(msg);
    return -1;
}

/**
 * @brief  free the inter-core audio effecter
 * @param  [in] hdl
 * @return 0/-1
 */
int aef_icore_free(aef_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    aef_icore_free_t *inp;
    struct aef_icore_ap_priv *priv;

    CHECK_PARAM(hdl, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_AEF_FREE, sizeof(aef_icore_free_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, aef_icore_free_t);
    inp->aef = hdl->aef;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);
    mblock_free(priv->in_mb);
    mblock_free(priv->out_mb);
    av_free(hdl->priv);
    av_free(hdl);

    return rc;
}

/**
 * @brief  init inter-core audio effecter
 * @return 0/-1
 */
int aef_icore_init()
{
    if (!g_aef_icore.init) {
        g_aef_icore.ipc = ipc_get(AEF_ICORE_CP_IDX);
        CHECK_RET_TAG_WITH_RET(g_aef_icore.ipc, -1);
        ipc_add_service(g_aef_icore.ipc, AEF_ICORE_IPC_SERIVCE_ID, NULL, NULL);

        g_aef_icore.init = 1;
    }

    return g_aef_icore.init ? 0 : -1;
}

#endif


