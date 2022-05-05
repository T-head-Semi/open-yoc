#include <stdio.h>
#include <errno.h>
#include <ble_os.h>
#include <aos/debug.h>
#include <aos/kernel.h>
#include <aos/ble.h>
#include <aos/aos.h>
#include "yoc/ais_client.h"
#include "yoc/ble_ais.h"
#include "bluetooth/hci.h"

#define TAG "AIS_CLIENT"

#define MTU_MAX 247
#define AIS_DISCOVER_MAX_RETRY  10
#define AIS_CLIENT_STATE_UPDATE_TIMEOUT (1000)//ms

enum {
    DISC_IDLE = 0,
    DISC_SRV,
    DISC_CHAR_READ,
    DISC_CHAR_WRITE,
    DISC_CHAR_WRITE_NR,
    DISC_DES,
    DISC_SUCCESS,
};

typedef struct _bt_uuid {
    uint8_t type;
} bt_uuid;

struct bt_le_conn_param g_quick_param = {
    0x0c,    //15ms
    0x0c,    //15ms
    0,
    400,
};

typedef struct {
    dev_addr_t conn_dev;
    //dev_addr_t conn_dev_last;
    conn_param_t conn_param;
    ais_cb  user_cb;
    //uint8_t connect_same_flag;
    uint8_t mode;
    uint8_t discovery_state;
    uint8_t ccc_flag;
    uint8_t discover_max_retry;
    k_timer_t ais_state_timer;
} ais_client_ctx;

extern ota_beacon_cb  g_ota_service_cb;

static struct bt_gatt_discover_params  discov_param = {0};
static struct bt_uuid_16 ais_primary_uuid = BT_UUID_INIT_16(0xFEB3);
static struct bt_uuid_16 ais_read_uuid = BT_UUID_INIT_16(0xFED4);
static struct bt_uuid_16 ais_write_uuid = BT_UUID_INIT_16(0xFED5);
static struct bt_uuid_16 ais_write_wo_resp_uuid = BT_UUID_INIT_16(0xFED7);
static struct bt_uuid_16 ccc_uuid = BT_UUID_INIT_16(0x2902);
static struct bt_gatt_subscribe_params subscribe_param;

static ais_client_ctx  g_ais_ctx = {0};
static ais_client_t    g_ais = {0};

static void device_find(const bt_addr_le_t *addr, s8_t rssi,
                        u8_t adv_type, struct net_buf_simple *buf)
{
    if (adv_type != BT_LE_ADV_NONCONN_IND && adv_type != BT_LE_ADV_IND) {
        return;
    }

    if (buf->len > 31) {
        return;
    }

    if (g_ota_service_cb) {
        g_ota_service_cb(addr, rssi, adv_type, buf->data, buf->len, 1);
    }

}

int ais_client_scan_start()
{

    struct bt_le_scan_param p = {0};

    p.type = BT_LE_SCAN_TYPE_PASSIVE,
    p.interval = 0x0060; //60ms
    p.window = 0x0030;//30ms
    p.options |= BT_LE_SCAN_OPT_FILTER_DUPLICATE;

    return bt_le_scan_start(&p, device_find);
}


static u8_t ais_client_notify(struct bt_conn *conn,
                              struct bt_gatt_subscribe_params *param,
                              const void *buf, u16_t len)
{
    if (g_ais_ctx.user_cb) {
        g_ais_ctx.user_cb(AIS_EVENT_NOTIFY, (uint8_t *)buf, len);
    }

    return BT_GATT_ITER_CONTINUE;
}



void conn_param_update(struct bt_conn *conn, u16_t interval, u16_t latency, u16_t timeout)
{
    LOGD(TAG, "LE conn param updated: int 0x%04x lat %d to %d\n", interval, latency, timeout);
}



static u8_t ais_discover_func(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr,
                              struct bt_gatt_discover_params *params)
{
    int err = 0;
    struct bt_conn_info info = {0x00};
    err = bt_conn_get_info(conn, &info);

    if (err) {
        LOGE(TAG, "Get conn info status faild %d", err);
        return BT_GATT_ITER_STOP;
    }

    if (attr == NULL) {
        return BT_GATT_ITER_STOP;
    }

    if (params->uuid == &ais_primary_uuid.uuid) {
        struct bt_gatt_service_val *value = attr->user_data;
        discov_param.uuid = &ais_read_uuid.uuid;
        discov_param.start_handle = attr->handle + 1;
        discov_param.end_handle   = value->end_handle;
        discov_param.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        g_ais.ais_profile.ais_handle = attr->handle;
        g_ais.ais_profile.ais_end_handle = value->end_handle;
        g_ais_ctx.discovery_state = DISC_SRV;
        bt_gatt_discover(conn, &discov_param);
        return BT_GATT_ITER_STOP;
    } else if (params->uuid == &ais_read_uuid.uuid) {
        discov_param.uuid = &ais_write_uuid.uuid;
        discov_param.start_handle = attr->handle + 1;
        discov_param.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        g_ais.ais_profile.read_char_handle = attr->handle + 1;
        g_ais_ctx.discovery_state = DISC_CHAR_READ;
        bt_gatt_discover(conn, &discov_param);
        return BT_GATT_ITER_STOP;
    } else if (params->uuid == &ais_write_uuid.uuid) {
        discov_param.uuid = &ais_write_wo_resp_uuid.uuid;
        discov_param.start_handle = attr->handle + 1;
        discov_param.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        g_ais.ais_profile.write_char_handle = attr->handle + 1;
        g_ais_ctx.discovery_state = DISC_CHAR_WRITE;
        bt_gatt_discover(conn, &discov_param);
        return BT_GATT_ITER_STOP;
    } else if (params->uuid == &ais_write_wo_resp_uuid.uuid) {
        discov_param.uuid = &ccc_uuid.uuid;
        discov_param.start_handle =  attr->handle + 1;
        discov_param.type = BT_GATT_DISCOVER_DESCRIPTOR;
        g_ais.ais_profile.write_nr_char_handle = attr->handle + 1;
        g_ais_ctx.discovery_state = DISC_CHAR_WRITE_NR;
        bt_gatt_discover(conn, &discov_param);
        return BT_GATT_ITER_STOP;
    } else if (params->uuid == &ccc_uuid.uuid) {
        if (!g_ais_ctx.ccc_flag) {
            subscribe_param.notify = ais_client_notify;
            subscribe_param.value = BT_GATT_CCC_NOTIFY;
            subscribe_param.ccc_handle = attr->handle;
            subscribe_param.value_handle = attr->handle - 1;
            g_ais_ctx.discovery_state = DISC_DES;
            err = bt_gatt_subscribe(conn, &subscribe_param);

            if (err && err != -EALREADY) {
                g_ais_ctx.discovery_state = DISC_IDLE;
                LOGE(TAG, "Subscribe failed (err %d)\r\n", err);
                return BT_GATT_ITER_STOP;
            }

            g_ais.ais_profile.notify_enabled = 1;
            g_ais_ctx.ccc_flag = 1;
            g_ais_ctx.discovery_state = DISC_SUCCESS;
            k_timer_stop(&g_ais_ctx.ais_state_timer);
            err = bt_conn_le_param_update(g_ais.conn_handle, &g_quick_param);

            if (err) {
                LOGE(TAG, "Conn update err %d", err);
                return BT_GATT_ITER_STOP;
            }

            dev_addr_t conn_mac_info = {0};
            memcpy(conn_mac_info.val, info.le.dst->a.val, 6);
            conn_mac_info.type =  info.le.dst->type;

            if (g_ais_ctx.user_cb) {
                g_ais_ctx.user_cb(AIS_EVENT_DISCOVER_SUCCEED, &conn_mac_info, sizeof(dev_addr_t));
            }

            return BT_GATT_ITER_STOP;
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

static int _ais_gatt_discover(struct bt_conn *conn)
{
    if (g_ais_ctx.discovery_state != DISC_IDLE) {
        LOGW(TAG, "discovery pending");
        return 0;
    }

    discov_param.uuid = &ais_primary_uuid.uuid;
    discov_param.func = ais_discover_func;
    discov_param.start_handle = 0x0001;
    discov_param.end_handle = 0xffff;
    discov_param.type = BT_GATT_DISCOVER_PRIMARY;
    g_ais_ctx.discovery_state = DISC_IDLE;
    return bt_gatt_discover(conn, &discov_param);
}

static void mtu_exchange_cb(struct bt_conn *conn, u8_t err,
                            struct bt_gatt_exchange_params *params)
{
    int ret = 0;

    if (err) {
        LOGE(TAG, "mtu response failed (err %u)\n", err);
        ret = bt_conn_disconnect(g_ais.conn_handle, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

        if (ret) {
            LOGE(TAG, "gatt disconnect fail %d", ret);
        }

    } else {
        g_ais.mtu  = bt_gatt_get_mtu(conn);
        g_ais_ctx.discovery_state = DISC_IDLE;
        _ais_gatt_discover(conn);
        k_timer_start(&g_ais_ctx.ais_state_timer, AIS_CLIENT_STATE_UPDATE_TIMEOUT);

        if (g_ais_ctx.user_cb) {
            g_ais_ctx.user_cb(AIS_EVENT_MTU_EXCHANGE,  &g_ais.mtu, 1);
        }
    }
}


static void proxy_connected(struct bt_conn *conn, u8_t err)
{
    struct bt_conn_info info = {0x0};

    int ret = bt_conn_get_info(conn, &info);

    if (ret) {
        LOGE(TAG, "Get conn info status faild %d", err);
        return;
    }

    if (info.role == BT_HCI_ROLE_SLAVE || !g_ais.conn_handle || g_ais.conn_handle != conn) {
        LOGE(TAG, "Unexpected conn %p", conn);
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return;
    }

    connect_status conn_status = {0};
    memcpy(conn_status.addr.val, info.le.dst->a.val, 6);
    conn_status.addr.type = info.le.dst->type;
    conn_status.status    = err;

    if (!err) {
        static struct bt_gatt_exchange_params params;
        params.func = mtu_exchange_cb;
        int ret = 0;
        ret = bt_gatt_exchange_mtu(conn, &params);

        if (ret) {
            LOGE(TAG, "mtu exchange faild %d", ret);
            return;
        }

#if 0
        if (!memcmp(&conn_status.addr, &g_ais_ctx.conn_dev_last, sizeof(dev_addr_t))) {
            g_ais_ctx.connect_same_flag  = 1;
        } else {
            memcpy(&g_ais_ctx.conn_dev_last, &conn_status.addr, sizeof(dev_addr_t));
        }
#endif
        g_ais_ctx.discover_max_retry = AIS_DISCOVER_MAX_RETRY;
    } else {
        bt_conn_unref(conn);
        g_ais.conn_handle = NULL;
    }

    LOGD(TAG, "Connect with mac: %02x:%02x:%02x:%02x:%02x:%02x type:%02x status %02x %02x\r\n", conn_status.addr.val[5], conn_status.addr.val[4],   \
         conn_status.addr.val[3], conn_status.addr.val[2], conn_status.addr.val[1], conn_status.addr.val[0], conn_status.addr.type, conn_status.status, err);

    if (g_ais_ctx.user_cb) {
        g_ais_ctx.user_cb(AIS_EVENT_CONN, &conn_status, sizeof(connect_status));
    }
}

static void proxy_disconnected(struct bt_conn *conn, u8_t reason)
{
    struct bt_conn_info info = {0x0};
    int err = 0;
    err = bt_conn_get_info(conn, &info);

    if (err) {
        LOGE(TAG, "Get conn info status faild %d", err);
        return;
    }

    if (info.role == BT_HCI_ROLE_SLAVE || !g_ais.conn_handle || g_ais.conn_handle != conn) {
        return;
    }

    dev_addr_t conn_mac_info = {0};
    memcpy(conn_mac_info.val, info.le.dst->a.val, 6);
    conn_mac_info.type =  info.le.dst->type;
    g_ais.conn_handle = NULL;
    g_ais_ctx.ccc_flag = 0;
    memset(&discov_param, 0x0, sizeof(struct bt_gatt_discover_params));
    memset(&subscribe_param, 0x0, sizeof(struct bt_gatt_subscribe_params));

    LOGD(TAG, "Disconnect with mac: %02x:%02x:%02x:%02x:%02x:%02x type:%02x %02x\r\n", conn_mac_info.val[5], conn_mac_info.val[4],   \
         conn_mac_info.val[3], conn_mac_info.val[2], conn_mac_info.val[1], conn_mac_info.val[0], conn_mac_info.type, reason);

    if (g_ais_ctx.user_cb) {
        g_ais_ctx.user_cb(AIS_EVENT_DISCONN, &conn_mac_info, sizeof(dev_addr_t));
    }
}


static struct bt_conn_cb ais_conn_callbacks = {
    .connected        = proxy_connected,
    .disconnected     = proxy_disconnected,
    .le_param_updated = conn_param_update,
};


static void _ais_state_timeout_cb(void *timer, void *arg)
{
    k_timer_stop(&g_ais_ctx.ais_state_timer);

    if (g_ais_ctx.discovery_state == DISC_SUCCESS) {
        return;
    }

    if (g_ais_ctx.discover_max_retry == 0) {
        LOGD(TAG, "Discovery retry timeout ,disconn then");
        ble_ais_client_disconn(&g_ais);
    } else {
        _ais_gatt_discover(g_ais.conn_handle);
        g_ais_ctx.discover_max_retry--;
        k_timer_start(&g_ais_ctx.ais_state_timer, AIS_CLIENT_STATE_UPDATE_TIMEOUT);
    }
}


ais_handle_t ble_ais_client_init(ais_cb event_cb)
{
    int ret = 0;
    ret = bt_enable(NULL);

    if (ret < 0 && ret != -EALREADY) {
        LOGE(TAG, "init ble stack faild %d", ret);
        return NULL;
    }

    memset(&g_ais_ctx, 0, sizeof(ais_client_ctx));
    memset(&g_ais, 0, sizeof(ais_client_t));
    //g_ais_ctx.state = STATE_IDLE;
    g_ais_ctx.user_cb = event_cb;
    g_ais.conn_handle = NULL;

    bt_conn_cb_register(&ais_conn_callbacks);

    k_timer_init(&g_ais_ctx.ais_state_timer, _ais_state_timeout_cb, NULL);
    return &g_ais;
}


void ble_ais_client_unregister()
{
    bt_conn_cb_unregister(&ais_conn_callbacks);
}

int ble_ais_client_conn(ais_handle_t handle, dev_addr_t *peer_addr, conn_param_t *param)
{
    if (!handle || !peer_addr || !param) {
        return -EINVAL;
    }

    struct bt_conn *conn = NULL;

    bt_addr_le_t peer;

    struct bt_le_conn_param conn_param;

    memcpy(peer.a.val, peer_addr->val, 6);

    peer.type = peer_addr->type;

    conn_param = *(struct bt_le_conn_param *)param;

    conn = bt_conn_create_le(&peer, &conn_param);

    if (conn) {
        handle->conn_handle = conn;
        /* stack will hold this connection, release directly */
        bt_conn_unref(conn);
    } else {
        return -1;
    }

    return 0;
}

int ble_ais_client_disconn(ais_handle_t handle)
{
    if (!handle || !handle->conn_handle) {
        return -1;
    }

    return bt_conn_disconnect(handle->conn_handle, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

int ble_ais_client_send(ais_handle_t handle, uint16_t attr_handle, const  char *data, int length, uint8_t mode, ais_send_cb *cb)
{
    uint32_t count = length;
    int ret = 0;
    uint16_t wait_timer = 0;

    if (!handle || !data || length <= 0 || !handle->conn_handle || !handle->ais_profile.notify_enabled) {
        return -1;
    }

    if (cb != NULL && cb->start != NULL) {
        cb->start(0, NULL);
    }

    while (count) {
        uint16_t send_count = (handle->mtu - 3) < count ? (handle->mtu - 3) : count;

        if (mode == GATT_WRITE) {
            return -1;
        } else if (mode == GATT_WRITE_WITHOUT_RESPONSE) {
            bt_gatt_write_without_response(handle->conn_handle, attr_handle, (uint8_t *)data, send_count, false);
        }

        if (ret == -ENOMEM) {
            wait_timer++;

            if (wait_timer >= 500) {
                return -1;
            }

            aos_msleep(1);
            continue;
        }

        if (ret) {
            if (cb != NULL && cb->end != NULL) {
                cb->end(ret, NULL);
            }

            return ret;
        }

        data += send_count;
        count -= send_count;
    }

    if (cb != NULL && cb->end != NULL) {
        cb->end(0, NULL);
    }

    return 0;
}