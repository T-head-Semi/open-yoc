/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _GATEWAY_OTA_UART_H
#define _GATEWAY_OTA_UART_H

//#include "ota_trans/ota_server_trans.h"

typedef void (*ota_server_event_cb)(uint8_t event, const void *event_data);

int ota_server_trans_hci_uart_init(ota_server_event_cb cb);
int ota_server_trans_hci_uart_prepare(slist_t *dev_list);
int ota_server_trans_hci_uart_disconnect();
int ota_server_trans_hci_uart_send(uint8_t* data,uint8_t len,uint8_t ack);
int ota_server_trans_hci_uart_reset();
int ota_server_trans_hci_uart_unregister();


#endif
