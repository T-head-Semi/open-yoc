/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_FFTXER_IPC) && CONFIG_FFTXER_IPC
#include "ipc.h"
#include <csi_core.h>
#include "avutil/mem_block.h"
#include "fft_icore.h"
#include "icore/fft_icore_internal.h"

#define TAG                     "fft_icore_ap"
#define FFT_MB_SIZE_DEFAULT     (2*1024)

struct fft_icore {
    size_t          size;      // N

    void            *fft;       // fftx_t
    void            *priv;     // priv only for ap/cpu0
};

struct fft_icore_ap_priv {
    mblock_t        *in_mb;
    mblock_t        *out_mb;
};

static struct {
    ipc_t           *ipc;
    int             init;
} g_fft_icore;

static int _ipc_cmd_send(icore_msg_t *data, int sync)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    msg.service_id      = FFT_ICORE_IPC_SERIVCE_ID;
    msg.command         = IPC_CMD_FFT_ICORE;
    msg.req_data        = data;
    msg.req_len         = ICORE_MSG_SIZE + data->size;
    msg.resp_data       = data;
    msg.resp_len        = ICORE_MSG_SIZE + data->size;
    msg.flag           |= sync;

    ipc_message_send(g_fft_icore.ipc, &msg, AOS_WAIT_FOREVER);

    return data->ret.code;
}

/**
 * @brief  alloc a inter-core fft
 * @param  [in] size : N of the fft
 * @return NULL on error
 */
fft_icore_t* fft_icore_new(size_t size)
{
    int rc;
    fft_icore_t *hdl               = NULL;
    struct fft_icore_ap_priv *priv = NULL;
    mblock_t *in_mb               = NULL, *out_mb = NULL;
    icore_msg_t *msg_new          = NULL;
    fft_icore_new_t *inp;

    CHECK_PARAM(size, NULL);
    hdl = aos_zalloc(sizeof(fft_icore_t));
    CHECK_RET_TAG_WITH_RET(hdl, NULL);

    in_mb = mblock_new(FFT_MB_SIZE_DEFAULT, 16);
    out_mb = mblock_new(FFT_MB_SIZE_DEFAULT, 16);
    CHECK_RET_TAG_WITH_GOTO(in_mb && out_mb, err);

    msg_new = icore_msg_new(ICORE_CMD_FFT_NEW, sizeof(fft_icore_new_t));
    CHECK_RET_TAG_WITH_GOTO(msg_new, err);

    priv = aos_zalloc(sizeof(struct fft_icore_ap_priv));
    CHECK_RET_TAG_WITH_GOTO(priv, err);

    inp = icore_get_msg(msg_new, fft_icore_new_t);
    inp->size = size;

    rc = _ipc_cmd_send(msg_new, MESSAGE_SYNC);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    priv->in_mb  = in_mb;
    priv->out_mb = out_mb;
    hdl->fft     = inp->fft;
    hdl->size    = size;
    hdl->priv    = priv;
    icore_msg_free(msg_new);

    return hdl;
err:
    mblock_free(in_mb);
    mblock_free(out_mb);
    icore_msg_free(msg_new);
    aos_free(hdl);
    aos_free(priv);
    return NULL;
}

/**
 * @brief  fft forward
 * @param  [in] hdl
 * @param  [in] in
 * @param  [in] out
 * @return 0/-1
 */
int fft_icore_forward(fft_icore_t *hdl, const int16_t *in, int16_t *out)
{
    int rc = -1;
    size_t size;
    icore_msg_t *msg;
    fft_icore_forward_t *inp;
    struct fft_icore_ap_priv *priv;

    CHECK_PARAM(hdl && in && out, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_FFT_FORWARD, sizeof(fft_icore_forward_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    size = hdl->size * sizeof(int16_t);
    rc = mblock_grow(priv->in_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    rc = mblock_grow(priv->out_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, fft_icore_forward_t);
    inp->fft = hdl->fft;
    inp->in  = priv->in_mb->data;
    inp->out = priv->out_mb->data;
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
 * @brief  fft backward
 * @param  [in] hdl
 * @param  [in] in
 * @param  [in] out
 * @return 0/-1
 */
int fft_icore_backward(fft_icore_t *hdl, const int16_t *in, int16_t *out)
{
    int rc = -1;
    size_t size;
    icore_msg_t *msg;
    fft_icore_backward_t *inp;
    struct fft_icore_ap_priv *priv;

    CHECK_PARAM(hdl && in && out, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_FFT_BACKWARD, sizeof(fft_icore_backward_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    size = hdl->size * sizeof(int16_t);
    rc = mblock_grow(priv->in_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    rc = mblock_grow(priv->out_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, fft_icore_backward_t);
    inp->fft = hdl->fft;
    inp->in  = priv->in_mb->data;
    inp->out = priv->out_mb->data;
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
 * @brief  free the inter-core fft
 * @param  [in] hdl
 * @return 0/-1
 */
int fft_icore_free(fft_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    fft_icore_free_t *inp;
    struct fft_icore_ap_priv *priv;

    CHECK_PARAM(hdl, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_FFT_FREE, sizeof(fft_icore_free_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, fft_icore_free_t);
    inp->fft = hdl->fft;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);
    mblock_free(priv->in_mb);
    mblock_free(priv->out_mb);
    aos_free(hdl->priv);
    aos_free(hdl);

    return rc;
}

/**
 * @brief  init the icore inter-core fft
 * @return 0/-1
 */
int fft_icore_init()
{
    if (!g_fft_icore.init) {
        g_fft_icore.ipc = ipc_get(FFT_ICORE_CP_IDX);
        CHECK_RET_TAG_WITH_RET(g_fft_icore.ipc, -1);
        ipc_add_service(g_fft_icore.ipc, FFT_ICORE_IPC_SERIVCE_ID, NULL, NULL);

        g_fft_icore.init = 1;
    }

    return g_fft_icore.init ? 0 : -1;
}

#endif


