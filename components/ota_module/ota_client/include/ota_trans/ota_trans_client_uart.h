/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _GATEWAY_OTA_UART_H
#define _GATEWAY_OTA_UART_H

#include "ais_ota/ais_ota_client.h"



int ais_ota_trans_uart_init(ota_server_event_cb cb);
int ais_ota_trans_uart_prepare(slist_t *dev_list);
int ais_ota_trans_uart_disconnect();
int ais_ota_trans_uart_send(uint8_t* data,uint8_t len,uint8_t ack);
int ais_ota_trans_uart_reset();
int ais_ota_trans_uart_unregister();


#endif
