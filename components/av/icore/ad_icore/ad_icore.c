/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_DECODER_IPC) && CONFIG_DECODER_IPC
#include "ipc.h"
#include <csi_core.h>
#include "ad_icore.h"
#include "icore/ad_icore_internal.h"

#define TAG                     "ad_icore_ap"

#define AD_ICORE_ES_SIZE         (16*1024)
#define AD_ICORE_FRAME_POOL_SIZE (64*1024)

struct ad_icore {
    void            *ad;      // ad_cls_t
    sf_t            sf;
    void            *priv;    // priv only for ap/cpu0
};

struct ad_icore_ap_priv {
    uint8_t         *pool;
    uint8_t         *pool_orip;
    size_t          pool_size;

    uint8_t         *es_data;
    uint8_t         *es_orip;
    size_t          es_size;

    icore_msg_t     *msg_dec; // malloc only once for opt
};

static struct {
    ipc_t           *ipc;
    int             init;
} g_ad_icore;

static int _ipc_cmd_send(icore_msg_t *data, int sync)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    msg.service_id      = AD_ICORE_IPC_SERIVCE_ID;
    msg.command         = IPC_CMD_AD_ICORE;
    msg.req_data        = data;
    msg.req_len         = ICORE_MSG_SIZE + data->size;
    msg.resp_data       = data;
    msg.resp_len        = ICORE_MSG_SIZE + data->size;
    msg.flag           |= sync;

    ipc_message_send(g_ad_icore.ipc, &msg, AOS_WAIT_FOREVER);

    return data->ret.code;
}

/**
 * @brief  init the icore audio decoder
 * @return 0/-1
 */
int ad_icore_init()
{
    if (!g_ad_icore.init) {
        g_ad_icore.ipc = ipc_get(AD_ICORE_CP_IDX);
        ipc_add_service(g_ad_icore.ipc, AD_ICORE_IPC_SERIVCE_ID, NULL, NULL);
        CHECK_RET_TAG_WITH_RET(g_ad_icore.ipc, -1);
        g_ad_icore.init = 1;
    }

    return g_ad_icore.init ? 0 : -1;
}

/**
* @brief  init the ad_icore config param
* @param  [in] ad_cnf
* @return 0/-1
*/
int ad_icore_conf_init(adi_conf_t *adi_cnf)
{
    //TODO:
    CHECK_PARAM(adi_cnf, -1);
    memset(adi_cnf, 0, sizeof(adi_conf_t));
    return 0;
}

/**
 * @brief  open/create one audio remote-ipc decoder
 * @param  [in] adi_cnf
 * @return NULL on error
 */
ad_icore_t* ad_icore_open(avcodec_id_t id, const adi_conf_t *adi_cnf)
{
    int rc;
    char *es_data, *es_orip      = NULL;
    char *pool, *pool_orip       = NULL;
    uint8_t *extradata           = NULL;
    uint8_t *extradata_orip      = NULL;
    ad_icore_t *hdl               = NULL;
    struct ad_icore_ap_priv *priv = NULL;
    icore_msg_t *msg             = NULL;
    icore_msg_t *msg_dec         = NULL;
    ad_icore_open_t *inp;

    if (!(adi_cnf && (id != AVCODEC_ID_UNKNOWN))) {
        LOGE(TAG, "param fail");
        return NULL;
    }

    hdl = av_zalloc(sizeof(ad_icore_t));
    CHECK_RET_TAG_WITH_RET(hdl, NULL);

    es_orip = av_zalloc(ICORE_ALIGN_BUFZ(AD_ICORE_ES_SIZE));
    CHECK_RET_TAG_WITH_GOTO(es_orip, err);
    es_data = ICORE_ALIGN(es_orip);

    pool_orip = av_zalloc(ICORE_ALIGN_BUFZ(AD_ICORE_FRAME_POOL_SIZE));
    CHECK_RET_TAG_WITH_GOTO(pool_orip, err);
    pool = ICORE_ALIGN(pool_orip);

    priv = av_zalloc(sizeof(struct ad_icore_ap_priv));
    CHECK_RET_TAG_WITH_GOTO(priv, err);

    msg_dec = icore_msg_new(ICORE_CMD_AD_DECODE, sizeof(ad_icore_decode_t));
    CHECK_RET_TAG_WITH_GOTO(msg_dec, err);

    msg = icore_msg_new(ICORE_CMD_AD_OPEN, sizeof(ad_icore_open_t));
    CHECK_RET_TAG_WITH_GOTO(msg, err);

    inp = icore_get_msg(msg, ad_icore_open_t);
    inp->id = id;
    if (adi_cnf->extradata && adi_cnf->extradata_size) {
        extradata_orip = av_zalloc(ICORE_ALIGN_BUFZ(adi_cnf->extradata_size));
        CHECK_RET_TAG_WITH_GOTO(extradata_orip, err);
        extradata = ICORE_ALIGN(extradata_orip);

        memcpy(extradata, adi_cnf->extradata, adi_cnf->extradata_size);
        inp->adi_cnf.extradata      = extradata;
        inp->adi_cnf.extradata_size = adi_cnf->extradata_size;
        csi_dcache_clean_invalid_range((uint32_t*)inp->adi_cnf.extradata, inp->adi_cnf.extradata_size);
    }

    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    priv->es_data   = (uint8_t*)es_data;
    priv->es_orip   = (uint8_t*)es_orip;
    priv->es_size   = AD_ICORE_ES_SIZE;
    priv->pool      = (uint8_t*)pool;
    priv->pool_orip = (uint8_t*)pool_orip;
    priv->pool_size = AD_ICORE_FRAME_POOL_SIZE;
    priv->msg_dec   = msg_dec;

    hdl->ad   = inp->ad;
    hdl->sf   = inp->sf;
    hdl->priv = priv;
    icore_msg_free(msg);
    av_free(extradata_orip);

    return hdl;
err:
    av_free(extradata_orip);
    av_free(hdl);
    av_free(es_orip);
    av_free(pool_orip);
    av_free(priv);
    icore_msg_free(msg);
    icore_msg_free(msg_dec);
    return NULL;
}

/**
* @brief  get the audio sample format
* @param  [in] hdl
* @param  [out] sf
* @return 0/-1
*/
int ad_icore_get_sf(ad_icore_t *hdl, sf_t *sf)
{
    int rc = -1;

    if (!(hdl && sf)) {
        return rc;
    }

    *sf = hdl->sf;
    return rc;
}


/**
 * @brief  decode one audio frame
 * @param  [in] hdl
 * @param  [out] frame : frame of pcm
 * @param  [out] got_frame : whether decode one frame
 * @param  [in]  pkt : data of encoded audio
 * @return -1 when err happens, otherwise the number of bytes consumed from the
 *         input avpacket is returned
 */
int ad_icore_decode(ad_icore_t *hdl, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    int rc = -1, ret;
    icore_msg_t *msg;
    ad_icore_decode_t *inp;
    struct ad_icore_ap_priv *priv;

    if (!(hdl && frame && got_frame && pkt && pkt->data && pkt->len && (pkt->len < AD_ICORE_ES_SIZE))) {
        return rc;
    }

    *got_frame = 0;
    priv       = hdl->priv;
    msg        = priv->msg_dec;
    inp        = icore_get_msg(msg, ad_icore_decode_t);
    memset(inp, 0, sizeof(ad_icore_decode_t));
    avframe_set_mempool(&inp->frame, priv->pool, priv->pool_size);
    inp->ad      = hdl->ad;
    inp->es_len  = pkt->len;
    inp->es_data = priv->es_data;
    memcpy(inp->es_data, pkt->data, pkt->len);
    csi_dcache_clean_invalid_range((uint32_t*)inp->es_data, inp->es_len);
    rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
    if (rc > 0 && inp->got_frame) {
        *got_frame = inp->got_frame;
        csi_dcache_invalid_range((uint32_t*)priv->pool, priv->pool_size);
        ret = avframe_copy_from(&inp->frame, frame);
        if (ret != 0) {
            rc = -1;
            LOGE(TAG, "frame copy failed, may be oom. ret = %d, mpool = %p, msize = %d, linesize = %d\n",
                 ret, frame->mpool, frame->msize, inp->frame.linesize[0]);
        }
    }

    return rc;
}

/**
 * @brief  reset the ad_icore
 * @param  [in] hdl
 * @return 0/-1
 */
int ad_icore_reset(ad_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;

    if (!hdl) {
        return rc;
    }

    msg  = icore_msg_new(ICORE_CMD_AD_RESET, sizeof(ad_icore_reset_t));
    if (msg) {
        ad_icore_reset_t *inp = icore_get_msg(msg, ad_icore_reset_t);
        inp->ad = hdl->ad;

        rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
        icore_msg_free(msg);
    }

    return rc;
}

/**
 * @brief  close/destroy audio remote-ipc decoder
 * @param  [in] hdl
 * @return 0/-1
 */
int ad_icore_close(ad_icore_t *hdl)
{
    int rc = -1;
    icore_msg_t *msg;
    struct ad_icore_ap_priv *priv;

    if (!hdl) {
        return rc;
    }

    priv = hdl->priv;
    msg  = icore_msg_new(ICORE_CMD_AD_CLOSE, sizeof(ad_icore_close_t));
    if (msg) {
        ad_icore_close_t *inp = icore_get_msg(msg, ad_icore_close_t);
        inp->ad = hdl->ad;

        rc = _ipc_cmd_send(msg, MESSAGE_SYNC);
        icore_msg_free(msg);
    }

    av_free(priv->es_orip);
    av_free(priv->pool_orip);
    icore_msg_free(priv->msg_dec);
    av_free(hdl->priv);
    av_free(hdl);

    return rc;
}

#endif


