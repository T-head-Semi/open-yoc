/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_MCAXER_IPC) && CONFIG_MCAXER_IPC
#include "ipc.h"
#include <csi_core.h>
#include "avutil/mem_block.h"
#include "mca_icore.h"
#include "icore/mca_icore_internal.h"

#define TAG                     "mca_icore_ap"
#define MCA_MB_SIZE_DEFAULT     (1*1024)

struct mca_icore {
    void            *mca;       // mcax_t
    void            *priv;     // priv only for ap/cpu0
};

struct mca_icore_ap_priv {
    mblock_t        *in_mb;
    mblock_t        *out_mb;
};

static struct {
    ipc_t           *ipc;
    int             init;
} g_mca_icore;

static int _ipc_cmd_send(icore_msg_t *data, int sync)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    msg.service_id      = MCA_ICORE_IPC_SERIVCE_ID;
    msg.command         = IPC_CMD_MCA_ICORE;
    msg.req_data        = data;
    msg.req_len         = ICORE_MSG_SIZE + data->size;
    msg.resp_data       = data;
    msg.resp_len        = ICORE_MSG_SIZE + data->size;
    msg.flag           |= sync;

    ipc_message_send(g_mca_icore.ipc, &msg, AOS_WAIT_FOREVER);

    return data->ret.code;
}

/**
 * @brief  alloc a inter-core mca
 * @param  [in] type : MCA_TYPE_XXX
 * @return NULL on error
 */
mca_icore_t* mca_icore_new(int type)
{
    int rc;
    mca_icore_t *hdl               = NULL;
    struct mca_icore_ap_priv *priv = NULL;
    mblock_t *in_mb               = NULL, *out_mb = NULL;
    icore_msg_t *msg_new          = NULL;
    mca_icore_new_t *inp;

    CHECK_PARAM(type, NULL);
    hdl = av_zalloc(sizeof(mca_icore_t));
    CHECK_RET_TAG_WITH_RET(hdl, NULL);

    in_mb = mblock_new(MCA_MB_SIZE_DEFAULT, 16);
    out_mb = mblock_new(MCA_MB_SIZE_DEFAULT, 16);
    CHECK_RET_TAG_WITH_GOTO(in_mb && out_mb, err);

    msg_new = icore_msg_new(ICORE_CMD_MCA_NEW, sizeof(mca_icore_new_t));
    CHECK_RET_TAG_WITH_GOTO(msg_new, err);

    priv = av_zalloc(sizeof(struct mca_icore_ap_priv));
    CHECK_RET_TAG_WITH_GOTO(priv, err);

    inp = icore_get_msg(msg_new, mca_icore_new_t);
    inp->type = type;

    rc = _ipc_cmd_send(msg_new, MESSAGE_SYNC);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    priv->in_mb  = in_mb;
    priv->out_mb = out_mb;
    hdl->mca     = inp->mca;
    hdl->priv    = priv;
    icore_msg_free(msg_new);

    return hdl;
err:
    mblock_free(in_mb);
    mblock_free(out_mb);
    icore_msg_free(msg_new);
    av_free(hdl);
    av_free(priv);
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
int mca_icore_iir_fxp32_coeff32_config(mca_icore_t *hdl, const fxp32_t *coeff)
{
    int rc = -1;
    icore_msg_t *msg;
    mca_icore_iir_fxp32_coeff32_config_t *inp;

    CHECK_PARAM(hdl && coeff, -1);
    msg  = icore_msg_new(ICORE_CMD_MCA_IIR_FXP32_COEFF32_CONFIG, sizeof(mca_icore_iir_fxp32_coeff32_config_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    inp = icore_get_msg(msg, mca_icore_iir_fxp32_coeff32_config_t);
    inp->mca = hdl->mca;
    memcpy((void*)inp->coeff, (const void*)coeff, 5 * sizeof(fxp32_t));

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);

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
int mca_icore_iir_fxp32(mca_icore_t *hdl, const fxp32_t *input, size_t input_size, fxp32_t yn1, fxp32_t yn2,
                        fxp32_t *output)
{
    int rc = -1;
    size_t size;
    icore_msg_t *msg;
    mca_icore_iir_fxp32_t *inp;
    struct mca_icore_ap_priv *priv;

    CHECK_PARAM(hdl && input && input_size && output, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_MCA_IIR_FXP32, sizeof(mca_icore_iir_fxp32_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    size = input_size * sizeof(fxp32_t);
    rc = mblock_grow(priv->in_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    rc = mblock_grow(priv->out_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, mca_icore_iir_fxp32_t);
    inp->mca        = hdl->mca;
    inp->input_size = input_size;
    inp->yn1        = yn1;
    inp->yn2        = yn2;
    inp->input      = priv->in_mb->data;
    inp->output     = priv->out_mb->data;
    memcpy((void*)inp->input, (const void*)input, size);
    csi_dcache_invalid_range((uint32_t*)inp->output, size);
    csi_dcache_clean_invalid_range((uint32_t*)inp->input, size);

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    if (rc == 0) {
        size = (input_size - 2) * sizeof(fxp32_t);
        memcpy((void*)output, (const void*)inp->output, size);
    }
    icore_msg_free(msg);

    return rc;
err:
    icore_msg_free(msg);
    return -1;
}

/**
 * @brief  free the inter-core mca
 * @param  [in] hdl
 * @return 0/-1
 */
int mca_icore_free(mca_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    mca_icore_free_t *inp;
    struct mca_icore_ap_priv *priv;

    CHECK_PARAM(hdl, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_MCA_FREE, sizeof(mca_icore_free_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, mca_icore_free_t);
    inp->mca = hdl->mca;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);
    mblock_free(priv->in_mb);
    mblock_free(priv->out_mb);
    av_free(hdl->priv);
    av_free(hdl);

    return rc;
}

/**
 * @brief  init the icore inter-core mca
 * @return 0/-1
 */
int mca_icore_init()
{
    if (!g_mca_icore.init) {
        g_mca_icore.ipc = ipc_get(MCA_ICORE_CP_IDX);
        CHECK_RET_TAG_WITH_RET(g_mca_icore.ipc, -1);
        ipc_add_service(g_mca_icore.ipc, MCA_ICORE_IPC_SERIVCE_ID, NULL, NULL);

        g_mca_icore.init = 1;
    }

    return g_mca_icore.init ? 0 : -1;
}
#endif



