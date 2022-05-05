/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */
#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_OTA)
#include "common/log.h"
#include "yoc/ais_server.h"
#include "yoc/ble_ais.h"
#include "aos/ble.h"
#include "host/conn_internal.h"

static ble_ais_ctx_t ble_ais_ctx;


static struct bt_gatt_attr _ais_srv_attrs[];
static struct bt_gatt_indicate_params *p_indicate = NULL;


#define TAG "BLE_AIS"



void _ais_server_disconnect(struct bt_conn *p_conn, u8_t err)
{
    if(!ble_ais_ctx.init_flag) {
        return;
    }
    connect_status conn_status = {0};
    memcpy(conn_status.addr.val,p_conn->le.resp_addr.a.val,6);
    conn_status.addr.type = p_conn->le.resp_addr.type;
    conn_status.status    = err;
    LOGD(TAG,"Disonnect with mac: %02x:%02x:%02x:%02x:%02x:%02x type:%02x status %02x ref %d %02x\r\n",conn_status.addr.val[5],conn_status.addr.val[4],   \
         conn_status.addr.val[3], conn_status.addr.val[2],conn_status.addr.val[1],conn_status.addr.val[0],conn_status.addr.type,conn_status.status,p_conn->ref,err);

    ble_ais_ctx.p_conn = NULL;

    if(ble_ais_ctx.user_cb) {
        ble_ais_ctx.user_cb(AIS_SERVER_GATT_EVENT_DISCONN, &conn_status, sizeof(connect_status));
    }
}


void _ais_server_connect(struct bt_conn *p_conn, u8_t err)
{
    int ret = 0;
    if(!ble_ais_ctx.init_flag) {
        ret = bt_conn_disconnect(p_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        if(ret) {
            LOGD(TAG,"Disconnect faild %d",ret);
        }
        return;
    }

    connect_status conn_status = {0};
    memcpy(conn_status.addr.val,p_conn->le.resp_addr.a.val,6);
    conn_status.addr.type = p_conn->le.resp_addr.type;
    conn_status.status    = err;

    if(!err) {
        ble_ais_ctx.p_conn = p_conn;
    }

    LOGD(TAG,"Connect with mac: %02x:%02x:%02x:%02x:%02x:%02x type:%02x status %02x ref %d %02x\r\n",conn_status.addr.val[5],conn_status.addr.val[4],   \
         conn_status.addr.val[3], conn_status.addr.val[2],conn_status.addr.val[1],conn_status.addr.val[0],conn_status.addr.type,conn_status.status,p_conn->ref,err);

    if(ble_ais_ctx.user_cb) {
        ble_ais_ctx.user_cb(AIS_SERVER_GATT_EVENT_CONN, &conn_status, sizeof(connect_status));
    }

}

static void _ais_indicate_rsp(struct bt_conn *conn, const struct bt_gatt_attr *attr, u8_t err, void *p_params)
{
    //TODO
    LOGD(TAG,"err 0x%02x", err);
    if (p_indicate) {
    }
}

static struct bt_gatt_indicate_params g_indicate;

static struct bt_conn_cb ais_conn_callbacks = {
    .connected = _ais_server_connect,
    .disconnected = _ais_server_disconnect,
};


static ssize_t _ais_server_read(struct bt_conn *p_conn, const struct bt_gatt_attr *p_attr,
                                void *buf, u16_t len, u16_t offset)
{
    u16_t *value = p_attr->user_data;

    LOGD(TAG,"len %d: %s", len, bt_hex(buf, len));

    return bt_gatt_attr_read(p_conn, p_attr, buf, len, offset, value, sizeof(*value));
}

static ssize_t _ais_service_write(struct bt_conn *p_conn, const struct bt_gatt_attr *p_attr,
                                  const void *p_buf, u16_t len, u16_t offset, u8_t flags)
{
    //LOGD(TAG,"len %d: %s", len, bt_hex(p_buf, len));

    if (len != 0) {
        if(ble_ais_ctx.user_cb) {
            ble_ais_ctx.user_cb(AIS_SERVER_GATT_EVENT_WRITE, p_buf, len);
        }
    }
    return len;
}

static ssize_t _ais_service_write_nr(struct bt_conn *p_conn, const struct bt_gatt_attr *p_attr,
                                     const void *p_buf, u16_t len, u16_t offset, u8_t flags)
{
    if (len != 0) {
        if(ble_ais_ctx.user_cb) {
            ble_ais_ctx.user_cb(AIS_SERVER_GATT_EVENT_WRITE, p_buf, len);
        }
    }
    return len;
}

static void _ais_service_ccc_cfg_changed(const struct bt_gatt_attr *p_attr, uint16_t value)
{
}

/* AIS OTA Service Declaration */
static struct bt_gatt_attr _ais_srv_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(AIS_SERVICE_UUID),

    BT_GATT_CHARACTERISTIC(AIS_READ_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, _ais_server_read, NULL, NULL),

    BT_GATT_CHARACTERISTIC(AIS_WRITE_UUID, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE,
                           NULL, _ais_service_write, NULL),

    BT_GATT_CHARACTERISTIC(AIS_INDICATE_UUID, BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, _ais_server_read, NULL, NULL),

    BT_GATT_CCC(_ais_service_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(AIS_WRITE_WO_RESP_UUID, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, _ais_service_write_nr, NULL),

    BT_GATT_CHARACTERISTIC(AIS_NOTIFY_UUID, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, _ais_server_read, NULL, NULL),

    BT_GATT_CCC(_ais_service_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service _ais_srv = BT_GATT_SERVICE(_ais_srv_attrs);






int ble_ais_server_init(ais_cb event_cb)
{
    int ret =0;
    if(ble_ais_ctx.init_flag) {
        return 0;
    }
    ret = bt_gatt_service_register(&_ais_srv);
    if(ret) {
        LOGE(TAG,"gatt register service faild");
        return ret;
    }
    bt_conn_cb_register(&ais_conn_callbacks);

    memset(&ble_ais_ctx, 0, sizeof(ble_ais_ctx));
    ble_ais_ctx.user_cb = event_cb;
    ble_ais_ctx.init_flag = 1;
    return 0;
}

void ble_ais_server_indicate(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len)
{
    ais_pdu_t msg;

    LOGD(TAG,"msg_id %02x %02x", msg_id, cmd);
    if (!p_indicate) {
        p_indicate = &g_indicate;
    }

    if (!p_indicate) {
        LOGE(TAG,"no mem");
        return;
    }

    memset(&msg, 0, sizeof(msg));
    msg.header.msg_id = msg_id;
    msg.header.cmd = cmd;
    msg.header.payload_len = len;
    if (p_msg) {
        memcpy(msg.payload, p_msg, len);
    }

    LOGD(TAG,"len %d: %s", len + 4, bt_hex(&msg, len + 4));

    //indicate._req
    p_indicate->attr = &_ais_srv_attrs[6];
    p_indicate->func = _ais_indicate_rsp;
    p_indicate->data = &msg;
    p_indicate->len = len + 4;
    if (ble_ais_ctx.p_conn) {
        bt_gatt_indicate(ble_ais_ctx.p_conn, p_indicate);
    }
}


void ble_ais_server_notify(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len)
{
    ais_pdu_t msg;

    LOGD(TAG,"msg_id %02x %02x", msg_id, cmd);

    memset(&msg, 0, sizeof(msg));
    msg.header.cmd = cmd;
    msg.header.msg_id = msg_id;
    msg.header.payload_len = len;
    if (p_msg) {
        memcpy(msg.payload, p_msg, len);
    }

    LOGD(TAG,"len %d: %s", len + 4, bt_hex(&msg, len + 4));

    if (ble_ais_ctx.p_conn) {
        bt_gatt_notify(ble_ais_ctx.p_conn, &_ais_srv_attrs[11], &msg, len + 4);
    }
}



int ble_ais_server_disconnect()
{
    if(ble_ais_ctx.p_conn) {
        return bt_conn_disconnect(ble_ais_ctx.p_conn, BT_HCI_ERR_LOCALHOST_TERM_CONN);
    }
    return 0;
}


