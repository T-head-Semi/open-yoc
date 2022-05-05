#ifndef __AIS_CLIENT_H
#define __AIS_CLIENT_H

#include <bluetooth/conn.h>
#include "bluetooth/gatt.h"
#include "bluetooth/uuid.h"
#include "yoc/ble_ais.h"

typedef struct _ais_client_profile {
    uint8_t notify_enabled;
    uint16_t ais_handle;
    uint16_t ais_end_handle;
    uint16_t read_char_handle;
    uint16_t write_char_handle;
    uint16_t write_nr_char_handle;
    uint16_t ccc_handle;
} ais_client_profile;

typedef struct _ais_client_t {
    struct bt_conn* conn_handle;
    uint16_t mtu;
    ais_client_profile ais_profile;
} ais_client_t;

typedef enum {
    AIS_EVENT_CONN,
    AIS_EVENT_DISCONN,
    AIS_EVENT_MTU_EXCHANGE,
    AIS_EVENT_DISCOVER_SUCCEED,
    AIS_EVENT_NOTIFY,
} ais_client_event_en;



typedef ais_client_t* ais_handle_t;

typedef void (*ota_beacon_cb)(const bt_addr_le_t *addr, s8_t rssi,u8_t adv_type,void *user_data, uint16_t len,uint8_t type);

typedef struct _ais_send_cb {
    void (*start)(int err, void *cb_data);
    void (*end)(int err, void *cb_data);
} ais_send_cb;

ais_handle_t ble_ais_client_init(ais_cb event_cb);
void ble_ais_client_unregister();
int ble_ais_client_conn(ais_handle_t handle, dev_addr_t *peer_addr, conn_param_t *param);
int ble_ais_client_disconn(ais_handle_t handle);
int ble_ais_client_send(ais_handle_t handle,uint16_t attr_handle, const  char *data, int length, uint8_t mode, ais_send_cb *cb);

#endif