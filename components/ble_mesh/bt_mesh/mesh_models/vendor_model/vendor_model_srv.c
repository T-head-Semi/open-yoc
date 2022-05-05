/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <api/mesh.h>
#include "misc/util.h"
#include "misc/dlist.h"
#include "misc/dlist.h"

#include "vendor/vendor_model_srv.h"
#include "mesh_model/mesh_model.h"
#include "inc/mesh.h"
#include "common/log.h"

#define TAG "VENDOR"
#if defined(CONFIG_BT_MESH_MODEL_VENDOR_SRV)
extern u8_t bt_mesh_default_ttl_get(void);
extern u8_t get_remain_byte(S_MESH_STATE *p_state, bool is_ack);
extern uint8_t mesh_check_tid(u16_t src_addr, u8_t tid);
extern u32_t get_transition_time(u8_t byte);
extern long long aos_now_ms(void);

static void _vendor_model_retry_timer_cb(void *p_timer, void *args);

#define DEF_GET_VERSION_RETRY_TIMEOUT (1000)
#define TTL_NOT_SET 0xFF

/**
 * g_vnd_msg_list was used to save the unconfirmed vendor messages
 * g_vnd_msg_timer was used to control when the vendor messages saved in g_vnd_msg_list will be resent
 * */
static sys_dlist_t g_vnd_msg_list;
static struct k_timer g_vnd_msg_timer;




/**
 * vendor model publish context, alloc maximum message buffer size
 * Ethan: do we need to shrink the message size here?
 * */
struct bt_mesh_model_pub g_vendor_srv_model_alibaba_pub = {
    .msg = NET_BUF_SIMPLE(3 + 377 + 4), // allocate maximum payload size
};

/** @def _vendor_model_msg_gen_tid
 *
 *  @brief generate tid used in vendor model message
 *
 *  @param NULL
 *
 *  @return tid with range of [0x80, 0xff]
 */
u8_t _vendor_model_msg_gen_tid(void)
{
    static u8_t tid = 0x80;

    return (tid++ | 0x80);
}


/** @def vendor_model_init
 *
 *  @brief vendor model server related global parameter initialization
 *
 *  @param NULL
 *
 *  @return always return true
 */
static u16_t vendor_model_init(void)
{
    static bool init_flag = false;

    if (!init_flag) {
        LOGI(TAG, "init g_vnd_msg_timer");
        k_timer_init(&g_vnd_msg_timer, _vendor_model_retry_timer_cb, &g_vnd_msg_list);
        sys_dlist_init(&g_vnd_msg_list);
    }

    init_flag = true;

    return true;
}

/** @def _vendor_model_msg_node_free
 *
 *  @brief free the vendor model message node struct's memory
 *
 *  @param pointer to the vendor model message node to be freed
 *
 *  @return 0 for success; negative for failure
 */
static s16_t _vendor_model_msg_node_free(vnd_model_msg_n *p_node)
{
    free(p_node);
    return 0;
}

/** @def _vendor_model_msg_node_generate
 *
 *  @brief duplicate vnd_model_msg and save to vnd_model_msg_n
 *
 *  @param pointer to the vendor model message to be duplicated
 *
 *  @return pointer to vnd_model_msg_n for success, NULL for failure
 */
#if 0
static vnd_model_msg_n *_vendor_model_msg_node_generate(vnd_model_msg *p_model_msg)
{
    vnd_model_msg_n *p_node = NULL;

    if (!p_model_msg->retry) {
        p_model_msg->retry = VENDOR_MODEL_MSG_DFT_RETRY_TIMES;
    } else if (p_model_msg->retry > VENDOR_MODEL_MSG_MAX_RETRY_TIMES) {
        p_model_msg->retry = VENDOR_MODEL_MSG_MAX_RETRY_TIMES;
    }

    p_node = malloc(sizeof(vnd_model_msg_n) + p_model_msg->len);

    if (!p_node) {
        LOGE(TAG, "malloc for vnd_model_msg_n failed");
        return NULL;
    }

    memcpy(&p_node->msg, p_model_msg, sizeof(vnd_model_msg));
    //LOGD(TAG, "p_node->msg:%p, data:%p, %p", &p_node->msg, &p_node->msg.data, &p_node->msg.data + 1);
    p_node->msg.data = (uint8_t *)&p_node->msg.data + 1;
    memcpy(p_node->msg.data, p_model_msg->data, p_model_msg->len);
    //LOGD(TAG, "p_model_msg->data:%p, %s", p_model_msg->data, bt_hex(p_model_msg->data, p_model_msg->len));
    //LOGD(TAG, "p_node->msg.data:%p, %s", p_node->msg.data, bt_hex(p_node->msg.data, p_node->msg.len));
    p_node->timeout = aos_now_ms() + VENDOR_MODEL_MSG_RETRY_PERIOD;

    p_node->left_retry =  p_model_msg->retry;

    return p_node;
}
#endif
/** @def _vendor_model_msg_list_append
 *
 *  @brief duplicate vnd_model_msg and append it to vendor model message list to be monitored
 *
 *  @param pointer to the vendor model message to be duplicated
 *
 *  @return 0 for success; negative for failure
 */
 #if 0
static s16_t _vendor_model_msg_list_append(vnd_model_msg *p_model_msg)
{
    vnd_model_msg_n *p_msg_node = NULL;

    p_msg_node = _vendor_model_msg_node_generate(p_model_msg);

    if (!p_msg_node) {
        return -2;
    }

    //LOGD(TAG, "append msg:%p, opid:%x, retry:%d, head:%p, node:%p", p_model_msg, p_model_msg->opid, p_model_msg->retry, &g_vnd_msg_list, &p_msg_node->node);

    sys_dlist_append(&g_vnd_msg_list, &p_msg_node->node);

    //Check retry timer, if timer is not started yet, start it
    if (!k_timer_is_started(&g_vnd_msg_timer)) {
        k_timer_start(&g_vnd_msg_timer, VENDOR_MODEL_MSG_RETRY_PERIOD);
    }

    return 0;
}
#endif
/** @def _vendor_model_retry_timer_cb
 *
 *  @brief timeout handler for the g_vnd_msg_timer
 *
 *  @param p_timer - pointer to the timer; args - pointer to g_vnd_msg_list
 *
 *  @return N/A
 */
static void _vendor_model_retry_timer_cb(void *p_timer, void *args)
{
    sys_dlist_t *p_head = (sys_dlist_t *)args;
    sys_dnode_t *p_node = NULL;
    u32_t nearest = VENDOR_MODEL_MSG_RETRY_PERIOD;
    vnd_model_msg_n *p_msg_node = NULL;
    vnd_model_msg *p_msg = NULL;

    //LOGD(TAG, "g_vnd_msg_timer timeout, p_head:%p", p_head);

    /**
     * 1. go through p_head
     * 2. resend the no responsed messages if timeout happens and refresh timeout value
     * */
    SYS_DLIST_FOR_EACH_NODE(p_head, p_node) {
        p_msg_node = CONTAINER_OF(p_node, vnd_model_msg_n, node);
        p_msg = &p_msg_node->msg;
        //LOGD(TAG, "msg:%p, opid:%d, left:%d", p_msg, p_msg->opid, p_msg_node->left_retry);

        /* Ethan: TODO - need to consider the wrap round case */
        if (p_msg_node->timeout <= aos_now_ms()) {
            //LOGD(TAG, "timeout - msg:%p, opid:%x, left:%d", p_msg, p_msg->opid, p_msg_node->left_retry);
            //vendor_srv_model_msg_send(p_msg);

            if (--p_msg_node->left_retry <= 0) {
                sys_dlist_remove(p_node);
                _vendor_model_msg_node_free((vnd_model_msg_n *)p_node);
                break;
            }

            p_msg_node->timeout = aos_now_ms() + VENDOR_MODEL_MSG_RETRY_PERIOD;
        } else {
            if (nearest > p_msg_node->timeout) {
                nearest = p_msg_node->timeout;
            }
        }
    }

    /* start new timer */
    if (!sys_dlist_is_empty(p_head)) {
        k_timer_start(&g_vnd_msg_timer, nearest);
        //LOGD(TAG, "restart retry timer, timeout:%d", nearest);
    } else {
        k_timer_stop(&g_vnd_msg_timer);
        //LOGD(TAG, "list empty, stop timer");
    }

    (void)p_msg;
    return;
}

/**
 * Ethan: need to add lock to prevent from multiple access
 * */

/** @def vendor_model_msg_send
 *
 *  @brief send the vendor model message with spec ttl
 *
 *  @param pointer to the message to be sent
 *
 *  @return 0 for success; negative for failure
 */
int ble_mesh_vendor_srv_model_msg_send_with_ttl(vnd_model_msg *model_msg, uint8_t ttl)
{
    struct bt_mesh_model_pub *p_pub = &g_vendor_srv_model_alibaba_pub;
    struct net_buf_simple *p_msg = p_pub->msg;
    s16_t err = -1;
    struct bt_mesh_msg_ctx ctx = {0};
    bool resend_flag = false;

    // Ethan: temp solution
    vendor_model_init();

    if (!model_msg || !model_msg->model) {
        LOGE(TAG, "Invalid argument\n");
        return err;
    }

    //LOGD(TAG, "p_model:%p, retry:%d\n", model_msg->model, model_msg->retry);

    /**
     * no need to duplicate the following messages
     * 1. retry <= 0 - the message won't want to be resent
     * 2. already duplicated or CONFIME/CONFIME_TG - p_model_msg->tid is in valid range [0x80, 0xff]
     * 3. SET_UNACK/CONFIME/CONFIME_TG/TRANSPARENT_MSG
     * 4. invalid tid (< 0x80)
     * */
    if ((model_msg->retry > 1) &&
        (model_msg->tid < 0x80) &&
        (model_msg->opid != VENDOR_OP_ATTR_SET_UNACK) &&
        (model_msg->opid != VENDOR_OP_ATTR_TRANS_MSG)
       ) {
        resend_flag = true;
    }

    /**
     * Ethan: if tid is invalid [0, 0x80), assign valid tid
     * only when oid is VENDOR_OP_ATTR_CONFIME or VENDOR_OP_ATTR_CONFIME_TG shall we keep tid as it is
     * */
    if (!(model_msg->tid) &&
        (model_msg->opid != VENDOR_OP_ATTR_TRANS_MSG)) {
        model_msg->tid = _vendor_model_msg_gen_tid();
    }

    //prepare buffer
    bt_mesh_model_msg_init(p_msg, BT_MESH_MODEL_OP_3(model_msg->opid, CONFIG_MESH_VENDOR_COMPANY_ID));
    net_buf_simple_add_u8(p_msg, model_msg->tid);

    if (model_msg->len) {
        net_buf_simple_add_mem(p_msg, model_msg->data, model_msg->len);
    }

    model_msg->retry--;

    //LOGD(TAG,"p_model_msg->data:%p, %d, %s", model_msg, model_msg->len, bt_hex(model_msg->data, model_msg->len));

    if (resend_flag) {
        //_vendor_model_msg_list_append(model_msg);
    }

    ctx.app_idx = model_msg->appkey_idx;
    ctx.net_idx = model_msg->netkey_idx;
    ctx.addr = model_msg->dst_addr;
    ctx.send_ttl = ttl;
    ctx.send_rel = 0;
	ctx.trans    = model_msg->trans;
	ctx.net_transmit = model_msg->net_transmit;
    err = bt_mesh_model_send(model_msg->model, &ctx, p_msg, NULL, NULL);

    if (err) {
        LOGE(TAG, "bt_mesh_model_publish err %d\n", err);
    } else {
        //LOGD(TAG, "Success!!!");
    }

    return err;
}


/** @def vendor_model_msg_send
 *
 *  @brief send the vendor model message with default ttl
 *
 *  @param pointer to the message to be sent
 *
 *  @return 0 for success; negative for failure
 */
int ble_mesh_vendor_srv_model_msg_send(vnd_model_msg *model_msg)
{
	return ble_mesh_vendor_srv_model_msg_send_with_ttl(model_msg,bt_mesh_default_ttl_get());
}

static uint8_t get_send_ttl(uint8_t cli_ttl, uint8_t recv_ttl)
{
   if(cli_ttl == TTL_NOT_SET || recv_ttl > cli_ttl) {
		return bt_mesh_default_ttl_get();
   } else {
		return cli_ttl - recv_ttl == 0 ? 0 : cli_ttl - recv_ttl  + 1;
   }
}


static void  _report_app_version(uint16_t dst_addr, uint8_t ack_ttl)
{
#if defined(CONFIG_OTA_CLIENT) && CONFIG_OTA_CLIENT > 0
    struct bt_mesh_model *vendor_model;
    vendor_model = ble_mesh_model_find(0, BT_MESH_MODEL_VND_MODEL_SRV, CONFIG_CID_TAOBAO);
    if (!vendor_model) {
        return;
    }

    vnd_model_msg  vnd_data = {0};
    uint8_t data[6]= {0};
    extern unsigned int ota_version_appver_get(void);
    uint32_t version = ota_version_appver_get();

    data[0] = ATTR_TYPE_REPORT_VERSION & 0xff;
    data[1] = (ATTR_TYPE_REPORT_VERSION >> 8) & 0xff;
    data[2] =  (version >> 24) & 0xff;
    data[3] =  (version >> 16) & 0xff;
    data[4] =  (version >> 8) & 0xff;
    data[5] =  version  & 0xff;

    vnd_data.netkey_idx = 0;
    vnd_data.appkey_idx = 0;
    vnd_data.dst_addr = dst_addr;
    vnd_data.model = vendor_model;
    vnd_data.opid = VENDOR_OP_ATTR_STATUS;
    vnd_data.data = data;
    vnd_data.retry = DEF_VERSION_REPORT_RETRY;
    vnd_data.len = sizeof(data);
	vnd_data.net_transmit = BT_MESH_TRANSMIT(5, 20);
    ble_mesh_vendor_srv_model_msg_send_with_ttl(&vnd_data,ack_ttl);
    return;
#endif
}



void  _report_sublist_overwrite_status(uint16_t dst_addr, uint8_t status)
{
    struct bt_mesh_model *vendor_model;
    vendor_model = ble_mesh_model_find(0, BT_MESH_MODEL_VND_MODEL_SRV, CONFIG_CID_TAOBAO);
    if (!vendor_model) {
        return;
    }

    vnd_model_msg  vnd_data = {0};
    uint8_t data[3]= {0};

    data[0] = ATTR_TYPE_OVERWRITE_SUBLIST & 0xff;
    data[1] = (ATTR_TYPE_OVERWRITE_SUBLIST >> 8) & 0xff;
    data[2] =  status;

    vnd_data.netkey_idx = 0;
    vnd_data.appkey_idx = 0;
    vnd_data.dst_addr = dst_addr;
    vnd_data.model = vendor_model;
    vnd_data.opid = VENDOR_OP_ATTR_STATUS;
    vnd_data.data = data;
    vnd_data.retry = DEF_VERSION_REPORT_RETRY;
    vnd_data.len = sizeof(data);
    ble_mesh_vendor_srv_model_msg_send(&vnd_data);
    return;
}

int vendor_set_cmd_process(uint16_t op_attr, struct bt_mesh_msg_ctx *p_ctx, struct net_buf_simple *p_buf)
{
    int ret = 0;
    //LOGD(TAG,"set attr:%02x",op_attr);
    switch (op_attr) {
    case ATTR_TYPE_OVERWRITE_SUBLIST: {
		extern int bt_mesh_mod_sublist_overwrite(struct net_buf_simple * buf);
        ret = bt_mesh_mod_sublist_overwrite(p_buf);
        if(ret) {
            LOGE(TAG,"Sublist overwrite faild 0x%02x",ret);
        }
        _report_sublist_overwrite_status(p_ctx->addr,ret);
    }
    break;
    }
    return ret;
}

/** @def _vendor_model_analyze
 *
 *  @brief analyze the received message and notify genie SDK
 *
 *  @param pointer to the received message (vendor model, context and the message buffer) and opid
 *
 *  @return if success return 0; if fails return error no.
 */
static s16_t _vendor_model_analyze(struct bt_mesh_model *p_model,
                                   struct bt_mesh_msg_ctx *p_ctx,
                                   struct net_buf_simple *p_buf,
                                   u8_t opid)
{
    vnd_model_msg msg;

    if (!p_model || !p_buf) {
        return MESH_ANALYZE_ARGS_ERROR;
    }

    /*
    if (p_buf->len < 3) {
        LOGE(TAG, "invalid buf len(%d)", p_buf->len);
        return MESH_ANALYZE_SIZE_ERROR;
    }*/
    uint8_t tid = net_buf_simple_pull_u8(p_buf);
    if (mesh_check_tid(p_ctx->addr, tid) != MESH_SUCCESS) {
        LOGE(TAG, "MESH_TID_REPEAT src_addr(0x%04x) tid(0x%02x)", p_ctx->addr, tid);
        return MESH_TID_REPEAT;
    }

    switch (opid) {
    case VENDOR_OP_ATTR_SET_ACK: {
        uint16_t op_attr = net_buf_simple_pull_le16(p_buf);
        vendor_set_cmd_process(op_attr,p_ctx, p_buf);
        return 0;  // dont pass through set ack message
    }
    break;
    default:
        break;
    }

    memset(&msg, 0, sizeof(vnd_model_msg));
    msg.opid = opid;
    msg.tid = tid;

    msg.len = p_buf->len;
    LOGD(TAG, "opcode:0x%x, tid:%d, len:%d", msg.opid, msg.tid, msg.len);

    if (msg.len) {
        msg.data = (u8_t *)p_buf->data;
        net_buf_simple_pull(p_buf, msg.len);
    } else {
        msg.data = NULL;
    }

	if(msg.opid != VENDOR_OP_ATTR_TRANS_MSG && msg.opid != VENDOR_OP_ATTR_CONFIRM) {
       return 0;
	}

	model_message vendor_msg = {0};
    vendor_msg.source_addr = p_ctx->addr;
	vendor_msg.trans =  p_ctx->trans;
    vendor_msg.ven_data.data_len = msg.len;
    vendor_msg.ven_data.user_data = msg.data;

    if(msg.opid == VENDOR_OP_ATTR_TRANS_MSG) {
        model_event(BT_MESH_MODEL_VENDOR_MESSAGES, (void *)&vendor_msg);
    } else if(msg.opid == VENDOR_OP_ATTR_CONFIRM) {
        model_event(BT_MESH_MODEL_VENDOR_MESH_CONFIRM, (void *)&vendor_msg);
	}

    return 0;
}

static  void sleep_random()
{
    uint8_t delay;
    bt_rand(&delay, 1);
    if(delay < 20) {
        delay = 20;
    } else if(delay > 100) {
        delay = 100;
    }
    k_sleep(delay);
}



static s16_t _vendor_model_analyze_get_status(struct bt_mesh_model *p_model,
        struct bt_mesh_msg_ctx *p_ctx,
        struct net_buf_simple *p_buf,
        u8_t opid)
{
    vnd_model_msg msg;
    uint16_t status = 0x00;
    static long long get_last =  0;
    if (!p_model || !p_buf) {
        return MESH_ANALYZE_ARGS_ERROR;
    }

    /*
    if (p_buf->len < 3) {
        LOGE(TAG, "invalid buf len(%d)", p_buf->len);
        return MESH_ANALYZE_SIZE_ERROR;
    }*/

    memset(&msg, 0, sizeof(vnd_model_msg));

    msg.opid = opid;
    msg.tid = net_buf_simple_pull_u8(p_buf);

    if (mesh_check_tid(p_ctx->addr, msg.tid) != MESH_SUCCESS) {
        LOGE(TAG, "MESH_TID_REPEAT src_addr(0x%04x) tid(0x%02x)", p_ctx->addr, msg.tid);
        return MESH_TID_REPEAT;
    }

    msg.len = p_buf->len;
    LOGI(TAG, "opcode:0x%x, tid:%d, len:%d %s", msg.opid, msg.tid, msg.len,bt_hex_real(p_buf->data,4));

    if(msg.len >= 2) {
        status = net_buf_simple_pull_le16(p_buf);
    } else {
        return MESH_ANALYZE_SIZE_ERROR;
    }

    switch (status) {
    case ATTR_TYPE_REPORT_VERSION : {
		if(BT_MESH_ADDR_IS_GROUP(p_ctx->recv_dst)) {
           sleep_random();
		}
		uint8_t cli_ttl = net_buf_simple_pull_u8(p_buf);
		uint8_t ack_ttl = get_send_ttl(cli_ttl, p_ctx->recv_ttl);
        long long get_now = aos_now_ms();
		if(get_last !=0 && get_now - get_last <= DEF_GET_VERSION_RETRY_TIMEOUT) {
           ack_ttl = bt_mesh_default_ttl_get();
		}
		get_last    = get_now;
        _report_app_version(p_ctx->addr, ack_ttl);
    }
    break;
    default:
        break;
    }

    return 0;
}


/** @def _vendor_model_get
 *
 *  @brief handle VENDOR_OP_ATTR_GET_STATUS message
 *
 *  @param pointer to the received message (vendor model, context and the message buffer)
 *
 *  @return N/A
 */
static void _vendor_model_get(struct bt_mesh_model *model,
                              struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf)
{
    _vendor_model_analyze_get_status(model, ctx, buf, VENDOR_OP_ATTR_GET_STATUS);
}

/** @def _vendor_model_set_ack
 *
 *  @brief handle VENDOR_OP_ATTR_SET_ACK message
 *
 *  @param pointer to the received message (vendor model, context and the message buffer)
 *
 *  @return N/A
 */
static void _vendor_model_set_ack(struct bt_mesh_model *model,
                                  struct bt_mesh_msg_ctx *ctx,
                                  struct net_buf_simple *buf)
{
    _vendor_model_analyze(model, ctx, buf, VENDOR_OP_ATTR_SET_ACK);
}

/** @def _vendor_model_confirm
 *
 *  @brief handle VENDOR_OP_ATTR_CONFIRM message
 *
 *  @param pointer to the received message (vendor model, context and the message buffer)
 *
 *  @return N/A
 */
static void _vendor_model_confirm(struct bt_mesh_model *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    struct net_buf_simple *buf)
{
    _vendor_model_analyze(model, ctx, buf, VENDOR_OP_ATTR_CONFIRM);
}

/** @def _vendor_model_set_unack
 *
 *  @brief handle VENDOR_OP_ATTR_SET_UNACK message
 *
 *  @param pointer to the received message (vendor model, context and the message buffer)
 *
 *  @return N/A
 */
static void _vendor_model_set_unack(struct bt_mesh_model *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    struct net_buf_simple *buf)
{
    _vendor_model_analyze(model, ctx, buf, VENDOR_OP_ATTR_SET_UNACK);
}


/** @def _vendor_model_transparent
 *
 *  @brief handle VENDOR_OP_ATTR_TRANS_MSG message
 *
 *  @param pointer to the received message (vendor model, context and the message buffer)
 *
 *  @return N/A
 */
static void _vendor_model_transparent(struct bt_mesh_model *model,
                                      struct bt_mesh_msg_ctx *ctx,
                                      struct net_buf_simple *buf)
{
    _vendor_model_analyze(model, ctx, buf, VENDOR_OP_ATTR_TRANS_MSG);
}


/** @def _vendor_model_autoconfig
*
*  @brief handle VENDOR_OP_ATTR_MESH_AUTOCONFIG message
*
*  @param pointer to the received message (vendor model, context and the message buffer)
*
*  @return N/A
*/
static void _vendor_model_autoconfig(struct bt_mesh_model *model,
                                     struct bt_mesh_msg_ctx *ctx,
                                     struct net_buf_simple *buf)
{
    uint8_t *data = (uint8_t *)buf->data;

    if (mesh_check_tid(ctx->addr, data[0]) != MESH_SUCCESS) {
        //LOGD(TAG, "MESH_TID_REPEAT src_addr(0x%04x) tid(0x%02x)", ctx->addr, data[0]);
        return;
    }

    model_message message = {0};
    message.source_addr = ctx->addr;
	message.trans =  ctx->trans;
    message.dst_addr = ctx->recv_dst;
    message.ven_data.data_len = buf->len;
    message.ven_data.user_data = buf;
    model_event(BT_MESH_MODEL_VENDOR_MESH_AUTOCONFIG, &message);
}

/** @def _vendor_model_autoconfig_get
*
*  @brief handle VENDOR_OP_ATTR_MESH_AUTOCONFIG message
*
*  @param pointer to the received message (vendor model, context and the message buffer)
*
*  @return N/A
*/
static void _vendor_model_autoconfig_get(struct bt_mesh_model *model,
        struct bt_mesh_msg_ctx *ctx,
        struct net_buf_simple *buf)
{

    uint8_t *data = (uint8_t *)buf->data;

    if (mesh_check_tid(ctx->addr, data[0]) != MESH_SUCCESS) {
        //LOGD(TAG, "MESH_TID_REPEAT src_addr(0x%04x) tid(0x%02x)", ctx->addr, data[0]);
        return;
    }

    model_message message = {0};
    message.source_addr = ctx->addr;
	message.trans =  ctx->trans;
    message.ven_data.data_len = buf->len;
    message.ven_data.user_data = buf;
    model_event(VENDOR_OP_ATTR_MESH_AUTOCONFIG_GET, &message);
}

int ble_mesh_vendor_srv_model_version_report(uint32_t version)
{
    struct bt_mesh_model *vendor_model;
    vendor_model = ble_mesh_model_find(0, BT_MESH_MODEL_VND_MODEL_SRV, CONFIG_CID_TAOBAO);
    if (!vendor_model) {
        return -1;
    }

    vnd_model_msg  vnd_data = {0};
    uint8_t data[6]= {0};

    data[0] = ATTR_TYPE_REPORT_VERSION && 0xff;
    data[1] = (ATTR_TYPE_REPORT_VERSION >> 8) & 0xff;
    data[2] =  (version >> 24) & 0xff;
    data[3] =  (version >> 16) & 0xff;
    data[4] =  (version >> 8) & 0xff;
    data[5] =  version  & 0xff;
    vnd_data.netkey_idx = 0;
    vnd_data.appkey_idx = 0;
    vnd_data.dst_addr = DEF_VERSION_REPORT_ADDR;
    vnd_data.model = vendor_model;
    vnd_data.opid = VENDOR_OP_ATTR_STATUS;
    vnd_data.data = data;
    vnd_data.retry = DEF_VERSION_REPORT_RETRY;
    vnd_data.len = sizeof(data);
    ble_mesh_vendor_srv_model_msg_send(&vnd_data);
    return 0;
}

/** @def g_vendor_model_alibaba_op
 *
 *  @brief vendor model operations struct
 *
 */


const struct bt_mesh_model_op g_vendor_srv_model_alibaba_op[VENDOR_SRV_MODEL_OPC_NUM] = {
    { BT_MESH_MODEL_OP_3(VENDOR_OP_ATTR_GET_STATUS, CONFIG_MESH_VENDOR_COMPANY_ID), 2, _vendor_model_get },
    { BT_MESH_MODEL_OP_3(VENDOR_OP_ATTR_SET_ACK, CONFIG_MESH_VENDOR_COMPANY_ID), 2, _vendor_model_set_ack },
    { BT_MESH_MODEL_OP_3(VENDOR_OP_ATTR_SET_UNACK, CONFIG_MESH_VENDOR_COMPANY_ID), 2, _vendor_model_set_unack },
    { BT_MESH_MODEL_OP_3(VENDOR_OP_ATTR_TRANS_MSG, CONFIG_MESH_VENDOR_COMPANY_ID), 1, _vendor_model_transparent },
    { BT_MESH_MODEL_OP_3(VENDOR_OP_ATTR_CONFIRM, CONFIG_MESH_VENDOR_COMPANY_ID), 1, _vendor_model_confirm},
    { BT_MESH_MODEL_OP_3(VENDOR_OP_ATTR_MESH_AUTOCONFIG, CONFIG_MESH_VENDOR_COMPANY_ID), 1, _vendor_model_autoconfig},
    { BT_MESH_MODEL_OP_3(VENDOR_OP_ATTR_MESH_AUTOCONFIG_GET, CONFIG_MESH_VENDOR_COMPANY_ID), 0, _vendor_model_autoconfig_get},
    BT_MESH_MODEL_OP_END,
};

#endif
