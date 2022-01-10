/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_EQXER_IPC) && CONFIG_EQXER_IPC
#include "ipc.h"
#include <csi_core.h>
#include "avutil/mem_block.h"
#include "eq_icore.h"
#include "icore/eq_icore_internal.h"

#define TAG                     "eq_icore_ap"
#define EQ_MB_SIZE_DEFAULT      (2*1024)

struct eq_icore {
    uint32_t        rate;
    uint8_t         eq_segments;

    void            *eq;       // eqx_t
    void            *priv;     // priv only for ap/cpu0
};

struct eq_icore_ap_priv {
    mblock_t        *in_mb;
    mblock_t        *out_mb;
};

static struct {
    ipc_t           *ipc;
    int             init;
} g_eq_icore;

static int _ipc_cmd_send(icore_msg_t *data, int sync)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    msg.service_id      = EQ_ICORE_IPC_SERIVCE_ID;
    msg.command         = IPC_CMD_EQ_ICORE;
    msg.req_data        = data;
    msg.req_len         = ICORE_MSG_SIZE + data->size;
    msg.resp_data       = data;
    msg.resp_len        = ICORE_MSG_SIZE + data->size;
    msg.flag           |= sync;

    ipc_message_send(g_eq_icore.ipc, &msg, AOS_WAIT_FOREVER);

    return data->ret.code;
}

/**
 * @brief  alloc a equalizer
 * @param  [in] rate
 * @param  [in] eq_segments : eq filter segment number max
 * @return NULL on error
 */
eq_icore_t* eq_icore_new(uint32_t rate, uint8_t eq_segments)
{
    int rc;
    eq_icore_t *hdl               = NULL;
    struct eq_icore_ap_priv *priv = NULL;
    mblock_t *in_mb              = NULL, *out_mb = NULL;
    icore_msg_t *msg_new         = NULL;
    eq_icore_new_t *inp;

    CHECK_PARAM(rate && eq_segments, NULL);
    hdl = av_zalloc(sizeof(eq_icore_t));
    CHECK_RET_TAG_WITH_RET(hdl, NULL);

    in_mb = mblock_new(EQ_MB_SIZE_DEFAULT, 16);
    out_mb = mblock_new(EQ_MB_SIZE_DEFAULT, 16);
    CHECK_RET_TAG_WITH_GOTO(in_mb && out_mb, err);

    msg_new = icore_msg_new(ICORE_CMD_EQ_NEW, sizeof(eq_icore_new_t));
    CHECK_RET_TAG_WITH_GOTO(msg_new, err);

    priv = av_zalloc(sizeof(struct eq_icore_ap_priv));
    CHECK_RET_TAG_WITH_GOTO(priv, err);

    inp = icore_get_msg(msg_new, eq_icore_new_t);
    inp->rate        = rate;
    inp->eq_segments = eq_segments;

    rc = _ipc_cmd_send(msg_new, MESSAGE_SYNC);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    priv->in_mb      = in_mb;
    priv->out_mb     = out_mb;
    hdl->eq          = inp->eq;
    hdl->rate        = rate;
    hdl->eq_segments = eq_segments;
    hdl->priv        = priv;
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
 * @brief  control whole eq filter enable/disable
 * @param  [in] hdl
 * @param  [in] enable
 * @return 0/-1
 */
int eq_icore_set_enable(eq_icore_t *hdl, uint8_t enable)
{
    int rc = -1;
    icore_msg_t *msg;
    eq_icore_seten_t *inp;

    CHECK_PARAM(hdl, -1);
    msg  = icore_msg_new(ICORE_CMD_EQ_SET_ENABLE, sizeof(eq_icore_seten_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, eq_icore_seten_t);
    inp->eq     = hdl->eq;
    inp->enable = enable;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);

    return rc;
}


/**
 * @brief  set config param to the filter segment
 * @param  [in] hdl
 * @param  [in] segid : id of the filter segment
 * @param  [in] param
 * @return
 */
int eq_icore_set_param(eq_icore_t *hdl, uint8_t segid, const eqfp_t *param)
{
    int rc = -1;
    icore_msg_t *msg;
    eq_icore_setpa_t *inp;

    CHECK_PARAM(hdl && param, -1);
    msg  = icore_msg_new(ICORE_CMD_EQ_SET_PARAM, sizeof(eq_icore_setpa_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, eq_icore_setpa_t);
    inp->eq    = hdl->eq;
    inp->segid = segid;
    memcpy(&inp->param, param, sizeof(eqfp_t));

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);

    return rc;
}

/**
 * @brief  process samples
 * @param  [in] hdl
 * @param  [in] in
 * @param  [in] out
 * @param  [in] nb_samples : samples per channel
 * @return 0/-1
 */
int eq_icore_process(eq_icore_t *hdl, const int16_t *in, int16_t *out, size_t nb_samples)
{
    int rc = -1;
    size_t size;
    icore_msg_t *msg;
    eq_icore_process_t *inp;
    struct eq_icore_ap_priv *priv;

    CHECK_PARAM(hdl && in && out && nb_samples, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_EQ_PROCESS, sizeof(eq_icore_process_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    size = nb_samples * sizeof(int16_t);
    rc = mblock_grow(priv->in_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    rc = mblock_grow(priv->out_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, eq_icore_process_t);
    inp->eq         = hdl->eq;
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
 * @brief  free the equalizer
 * @param  [in] hdl
 * @return 0/-1
 */
int eq_icore_free(eq_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    eq_icore_free_t *inp;
    struct eq_icore_ap_priv *priv;

    CHECK_PARAM(hdl, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_EQ_FREE, sizeof(eq_icore_free_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, eq_icore_free_t);
    inp->eq = hdl->eq;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);
    mblock_free(priv->in_mb);
    mblock_free(priv->out_mb);
    av_free(hdl->priv);
    av_free(hdl);

    return rc;
}

/**
 * @brief  init the icore equalizer
 * @return 0/-1
 */
int eq_icore_init()
{
    if (!g_eq_icore.init) {
        g_eq_icore.ipc = ipc_get(EQ_ICORE_CP_IDX);
        CHECK_RET_TAG_WITH_RET(g_eq_icore.ipc, -1);
        ipc_add_service(g_eq_icore.ipc, EQ_ICORE_IPC_SERIVCE_ID, NULL, NULL);

        g_eq_icore.init = 1;
    }

    return g_eq_icore.init ? 0 : -1;
}

#endif


