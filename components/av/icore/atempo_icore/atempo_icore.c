/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_ATEMPOER_IPC) && CONFIG_ATEMPOER_IPC
#include "ipc.h"
#include <csi_core.h>
#include "avutil/mem_block.h"
#include "atempo_icore.h"
#include "icore/atempo_icore_internal.h"

#define TAG                     "atempo_icore_ap"
#define ATEMPO_MB_SIZE_DEFAULT  (2*1024)

struct atempo_icore {
    uint32_t        rate;
    float           speed;        ///< suggest: 0.5 ~ 2.0;

    void            *atempo;      // atempox_t
    void            *priv;        // priv only for ap/cpu0
};

struct atempo_icore_ap_priv {
    mblock_t        *in_mb;
    mblock_t        *out_mb;
};

static struct {
    ipc_t           *ipc;
    int             init;
} g_atempo_icore;

static int _ipc_cmd_send(icore_msg_t *data, int sync)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    msg.service_id  = ATEMPO_ICORE_IPC_SERIVCE_ID;
    msg.command     = IPC_CMD_ATEMPO_ICORE;
    msg.req_data    = data;
    msg.req_len     = ICORE_MSG_SIZE + data->size;
    msg.resp_data   = data;
    msg.resp_len    = ICORE_MSG_SIZE + data->size;
    msg.flag       |= sync;

    ipc_message_send(g_atempo_icore.ipc, &msg, AOS_WAIT_FOREVER);

    return data->ret.code;
}

/**
 * @brief  alloc a inter-core audio atempoer
 * @param  [in] rate      : audio freq
 * @return NULL on error
 */
atempo_icore_t* atempo_icore_new(uint32_t rate)
{
    int rc;
    atempo_icore_t *hdl               = NULL;
    struct atempo_icore_ap_priv *priv = NULL;
    mblock_t *in_mb                   = NULL, *out_mb = NULL;
    icore_msg_t *msg_new              = NULL;
    atempo_icore_new_t *inp;

    CHECK_PARAM(rate, NULL);
    hdl  = av_zalloc(sizeof(atempo_icore_t));
    priv = av_zalloc(sizeof(struct atempo_icore_ap_priv));
    CHECK_RET_TAG_WITH_GOTO(hdl && priv, err);

    in_mb = mblock_new(ATEMPO_MB_SIZE_DEFAULT, 16);
    out_mb = mblock_new(ATEMPO_MB_SIZE_DEFAULT, 16);
    CHECK_RET_TAG_WITH_GOTO(in_mb && out_mb, err);

    msg_new = icore_msg_new(ICORE_CMD_ATEMPO_NEW, sizeof(atempo_icore_new_t));
    CHECK_RET_TAG_WITH_GOTO(msg_new, err);

    inp = icore_get_msg(msg_new, atempo_icore_new_t);
    inp->rate = rate;

    rc = _ipc_cmd_send(msg_new, MESSAGE_SYNC);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    priv->in_mb  = in_mb;
    priv->out_mb = out_mb;
    hdl->atempo  = inp->atempo;
    hdl->rate    = rate;
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
 * @brief  write samples to the inter-core audio atempoer
 * @param  [in] hdl
 * @param  [in] in
 * @param  [in] nb_samples : samples per channel
 * @return 0/-1
 */
int atempo_icore_write(atempo_icore_t *hdl, const int16_t *in, size_t nb_samples)
{
    int rc = -1;
    size_t size;
    icore_msg_t *msg;
    atempo_icore_write_t *inp;
    struct atempo_icore_ap_priv *priv;

    CHECK_PARAM(hdl && in && nb_samples, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_ATEMPO_WRITE, sizeof(atempo_icore_write_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    size = nb_samples * sizeof(int16_t);
    rc = mblock_grow(priv->in_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, atempo_icore_write_t);
    inp->atempo     = hdl->atempo;
    inp->nb_samples = nb_samples;
    inp->in         = priv->in_mb->data;
    memcpy((void*)inp->in, (const void*)in, size);
    csi_dcache_clean_invalid_range((uint32_t*)inp->in, size);

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);

    return rc;
err:
    icore_msg_free(msg);
    return -1;
}

/**
 * @brief  read samples from the inter-core audio atempoer
 * @param  [in] hdl
 * @param  [in] out
 * @param  [in] nb_samples : samples per channel
 * @return num of read samples, -1 on error
 */
int atempo_icore_read(atempo_icore_t *hdl, int16_t *out, size_t nb_samples)
{
    int rc = -1;
    size_t size;
    icore_msg_t *msg;
    atempo_icore_read_t *inp;
    struct atempo_icore_ap_priv *priv;

    CHECK_PARAM(hdl && out && nb_samples, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_ATEMPO_READ, sizeof(atempo_icore_read_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    size = nb_samples * sizeof(int16_t);
    rc = mblock_grow(priv->out_mb, ICORE_ALIGN_BUFZ(size));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, atempo_icore_read_t);
    inp->atempo     = hdl->atempo;
    inp->nb_samples = nb_samples;
    inp->out        = priv->out_mb->data;
    csi_dcache_invalid_range((uint32_t*)inp->out, size);

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    if (rc > 0) {
        memcpy((void*)out, (const void*)inp->out, rc * sizeof(int16_t));
    }
    icore_msg_free(msg);

    return rc;
err:
    icore_msg_free(msg);
    return -1;
}

/**
 * @brief  set_speed the inter-core audio atempoer
 * @param  [in] hdl
 * @param  [in] speed
 * @return 0/-1
 */
int atempo_icore_set_speed(atempo_icore_t *hdl, float speed)
{
    int rc = -1;
    icore_msg_t *msg;
    atempo_icore_set_speed_t *inp;
    struct atempo_icore_ap_priv *priv;

    CHECK_PARAM(hdl && speed > 0, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_ATEMPO_SET_SPEED, sizeof(atempo_icore_set_speed_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    inp = icore_get_msg(msg, atempo_icore_set_speed_t);
    inp->atempo = hdl->atempo;
    inp->speed  = speed;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    if (rc == 0) {
        hdl->speed = speed;
    }
    icore_msg_free(msg);

    return rc;
}

/**
 * @brief  flush the inter-core audio atempoer
 * @param  [in] hdl
 * @return 0/-1
 */
int atempo_icore_flush(atempo_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    atempo_icore_flush_t *inp;
    struct atempo_icore_ap_priv *priv;

    CHECK_PARAM(hdl, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_ATEMPO_FLUSH, sizeof(atempo_icore_flush_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    inp = icore_get_msg(msg, atempo_icore_flush_t);
    inp->atempo = hdl->atempo;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);

    return rc;
}

/**
 * @brief  free the inter-core audio atempoer
 * @param  [in] hdl
 * @return 0/-1
 */
int atempo_icore_free(atempo_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    atempo_icore_free_t *inp;
    struct atempo_icore_ap_priv *priv;

    CHECK_PARAM(hdl, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_ATEMPO_FREE, sizeof(atempo_icore_free_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, atempo_icore_free_t);
    inp->atempo = hdl->atempo;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);
    mblock_free(priv->in_mb);
    mblock_free(priv->out_mb);
    av_free(hdl->priv);
    av_free(hdl);

    return rc;
}

/**
 * @brief  init inter-core audio atempoer
 * @return 0/-1
 */
int atempo_icore_init()
{
    if (!g_atempo_icore.init) {
        g_atempo_icore.ipc = ipc_get(ATEMPO_ICORE_CP_IDX);
        CHECK_RET_TAG_WITH_RET(g_atempo_icore.ipc, -1);
        ipc_add_service(g_atempo_icore.ipc, ATEMPO_ICORE_IPC_SERIVCE_ID, NULL, NULL);

        g_atempo_icore.init = 1;
    }

    return g_atempo_icore.init ? 0 : -1;
}

#endif



