/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _OTA_SERVER_TRANS_GATT_H
#define _OTA_SERVER_TRANS_GATT_H

#include "yoc/ble_ais.h"

typedef void (*ota_server_event_cb)(uint8_t event, const void *event_data);


int ota_server_trans_gatt_init(ota_server_event_cb cb);
int ota_server_trans_gatt_send(uint8_t* data,uint8_t len,uint8_t ack);
int ota_server_trans_gatt_prepare(slist_t *dev_list);
int ota_server_trans_gatt_connect(dev_addr_t *dev);
int ota_server_trans_gatt_disconnect();
int ota_server_trans_gatt_unregister();


#endif
