#ifndef __AIS_SERVER_H
#define __AIS_SERVER_H
#include "yoc/ble_ais.h"

typedef enum {
    AIS_SERVER_GATT_EVENT_CONN,
    AIS_SERVER_GATT_EVENT_DISCONN,
    AIS_SERVER_GATT_EVENT_MTU_EXCHANGE,
    AIS_SERVER_GATT_EVENT_DISCOVER_SUCCEED,
    AIS_SERVER_GATT_EVENT_NOTIFY,
    AIS_SERVER_GATT_EVENT_WRITE,
} ais_server_event_en;

int  ble_ais_server_init(ais_cb cb);
void ble_ais_server_indicate(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len);
void ble_ais_server_notify(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len);
int ble_ais_server_disconnect();

#endif
