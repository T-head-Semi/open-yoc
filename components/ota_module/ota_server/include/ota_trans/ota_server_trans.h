#ifndef __OTA_SERVER_TRANS__
#define __OTA_SERVER_TRANS__

#include "k_types.h"
#include "aos/list.h"


typedef void (*ota_server_event_cb)(uint8_t event, const void *event_data);

int ota_server_trans_init(uint8_t channel, ota_server_event_cb cb);
int ota_server_trans_prepare( uint8_t channel, slist_t *dev_list);
int ota_server_trans_send(uint8_t channel, uint8_t* data,uint8_t len,uint8_t ack);
int ota_server_trans_disconnect(uint8_t channel);
int ota_server_trans_unregister(uint8_t channel);
int ota_server_trans_reset(uint8_t channel);




#endif

