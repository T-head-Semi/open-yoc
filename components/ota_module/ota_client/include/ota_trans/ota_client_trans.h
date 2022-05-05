#ifndef __OTA_TRANS_H__
#define __OTA_TRANS_H__
#include "stdint.h"


typedef void (*ota_client_event_cb)(uint8_t event,const void *event_data, uint8_t len);


typedef enum {
    OTA_CLIENT_EVENT_CONN,
    OTA_CLIENT_EVENT_DISCONN,
    OTA_CLIENT_EVENT_MTU_EXCHANGE,
    OTA_CLIENT_EVENT_DISCOVER_SUCCEED,
    OTA_CLIENT_EVENT_NOTIFY,
    OTA_CLIENT_EVENT_WRITE,
} ota_client_event_en;

int ota_client_trans_init(ota_client_event_cb cb);
int ota_client_trans_send(uint8_t channel, uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack);
int ota_client_trans_disconnect(uint8_t channel);

#endif
