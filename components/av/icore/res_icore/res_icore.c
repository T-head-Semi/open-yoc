/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_RESAMPLER_IPC) && CONFIG_RESAMPLER_IPC
#include "ipc.h"
#include <csi_core.h>
#include "avutil/mem_block.h"
#include "res_icore.h"
#include "icore/res_icore_internal.h"
#include "swresample/resample.h"

#define TAG                     "res_icore_ap"
#define RES_MB_SIZE_DEFAULT     (1*1024)

struct res_icore {
    uint32_t        irate;
    uint32_t        orate;
    uint8_t         bits;
    uint8_t         channels;

    void            *r;        // resx_t
    void            *priv;     // priv only for ap/cpu0
};

struct res_icore_ap_priv {
    mblock_t        *in_mb;
    mblock_t        *out_mb;
};

static struct {
    ipc_t           *ipc;
    int             init;
} g_res_icore;

static int _ipc_cmd_send(icore_msg_t *data, int sync)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    msg.service_id  = RES_ICORE_IPC_SERIVCE_ID;
    msg.command     = IPC_CMD_RES_ICORE;
    msg.req_data        = data;
    msg.req_len         = ICORE_MSG_SIZE + data->size;
    msg.resp_data        = data;
    msg.resp_len         = ICORE_MSG_SIZE + data->size;
    msg.flag       |= sync;

    ipc_message_send(g_res_icore.ipc, &msg, AOS_WAIT_FOREVER);

    return data->ret.code;
}

/**
 * @brief  new a resampler
 * @param  [in] irate    : input sample rate
 * @param  [in] orate    : output sample rate
 * @param  [in] channels : 1/2, mono/stereo, support mono now only
 * @param  [in] bits     : 16 only
 * @return NULL on error
 */
res_icore_t *res_icore_new(uint32_t irate, uint32_t orate, uint8_t channels, uint8_t bits)
{
    int rc;
    res_icore_t *hdl               = NULL;
    struct res_icore_ap_priv *priv = NULL;
    mblock_t *in_mb               = NULL, *out_mb = NULL;
    icore_msg_t *msg_new          = NULL;
    res_icore_new_t *inp;

    CHECK_PARAM(irate && orate && (channels == 1) && bits, NULL);
    hdl = av_zalloc(sizeof(res_icore_t));
    CHECK_RET_TAG_WITH_RET(hdl, NULL);

    in_mb = mblock_new(RES_MB_SIZE_DEFAULT, 16);
    out_mb = mblock_new(RES_MB_SIZE_DEFAULT, 16);
    CHECK_RET_TAG_WITH_GOTO(in_mb && out_mb, err);

    msg_new = icore_msg_new(ICORE_CMD_RES_NEW, sizeof(res_icore_new_t));
    CHECK_RET_TAG_WITH_GOTO(msg_new, err);

    priv = av_zalloc(sizeof(struct res_icore_ap_priv));
    CHECK_RET_TAG_WITH_GOTO(priv, err);

    inp = icore_get_msg(msg_new, res_icore_new_t);
    inp->irate    = irate;
    inp->orate    = orate;
    inp->channels = channels;
    inp->bits     = bits;

    rc = _ipc_cmd_send(msg_new, MESSAGE_SYNC);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    priv->in_mb   = in_mb;
    priv->out_mb  = out_mb;
    hdl->irate    = irate;
    hdl->orate    = orate;
    hdl->channels = channels;
    hdl->bits     = bits;
    hdl->r        = inp->r;
    hdl->priv     = priv;
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
 * @brief  get the max out samples number per channel
 * @param  [in] irate
 * @param  [in] orate
 * @param  [in] nb_isamples : number of input samples available in one channel
 * @return -1 on error
 */
int res_icore_get_osamples_max(uint32_t irate, uint32_t orate, size_t nb_isamples)
{
    return resx_get_osamples_max(irate, orate, nb_isamples);
}

/**
 * @brief  convert nb_samples from src to dst sample format
 * @param  [in] hdl
 * @param  [in] out
 * @param  [in] nb_osamples : amount of space available for output in samples per channel
 * @param  [in] in
 * @param  [in] nb_isamples : number of input samples available in one channel
 * @return number of samples output per channel, -1 on error
 */
int res_icore_convert(res_icore_t *hdl, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, rc = -1;
    size_t isize, osize;
    icore_msg_t *msg;
    res_icore_convert_t *inp;
    struct res_icore_ap_priv *priv;

    CHECK_PARAM(hdl && in && out && nb_isamples && nb_osamples, -1);
    for (i = 0; i < hdl->channels; i++) {
        if (!(in[i] && out[i])) {
            LOGE(TAG, "out/in params is err.");
            return -1;
        }
    }

    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_RES_CONVERT, sizeof(res_icore_convert_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);

    isize = nb_isamples * hdl->bits / 8;
    rc = mblock_grow(priv->in_mb, ICORE_ALIGN_BUFZ(isize));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    osize = nb_osamples * hdl->bits / 8;
    rc = mblock_grow(priv->out_mb, ICORE_ALIGN_BUFZ(osize));
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    inp = icore_get_msg(msg, res_icore_convert_t);
    inp->r           = hdl->r;
    inp->nb_isamples = nb_isamples;
    inp->nb_osamples = nb_osamples;
    inp->in          = priv->in_mb->data;
    inp->out         = priv->out_mb->data;
    memcpy((void*)inp->in, (const void*)in[0], isize);
    csi_dcache_clean_invalid_range((uint32_t*)inp->in, isize);

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    if (rc > 0) {
        osize = rc * hdl->bits / 8;
        csi_dcache_invalid_range((uint32_t*)inp->out, osize);
        memcpy((void*)out[0], (const void*)inp->out, osize);
    }
    icore_msg_free(msg);

    return rc;
err:
    icore_msg_free(msg);
    return -1;
}

/**
 * @brief  free the rualizer
 * @param  [in] hdl
 * @return 0/-1
 */
int res_icore_free(res_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    res_icore_free_t *inp;
    struct res_icore_ap_priv *priv;

    CHECK_PARAM(hdl, -1);
    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_RES_FREE, sizeof(res_icore_free_t));
    CHECK_RET_TAG_WITH_RET(msg, -1);
    inp = icore_get_msg(msg, res_icore_free_t);
    inp->r = hdl->r;

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    icore_msg_free(msg);
    mblock_free(priv->in_mb);
    mblock_free(priv->out_mb);
    av_free(hdl->priv);
    av_free(hdl);

    return rc;
}

/**
 * @brief  init the icore resample
 * @return 0/-1
 */
int res_icore_init()
{
    if (!g_res_icore.init) {
        g_res_icore.ipc = ipc_get(RES_ICORE_CP_IDX);
        CHECK_RET_TAG_WITH_RET(g_res_icore.ipc, -1);
        ipc_add_service(g_res_icore.ipc, RES_ICORE_IPC_SERIVCE_ID, NULL, NULL);

        g_res_icore.init = 1;
    }

    return g_res_icore.init ? 0 : -1;
}

#endif


