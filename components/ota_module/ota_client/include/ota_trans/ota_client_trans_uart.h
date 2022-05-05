/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _GATEWAY_OTA_UART_H
#define _GATEWAY_OTA_UART_H

#include "aos/list.h"

typedef void (*ota_client_event_cb)(uint8_t event,const void *event_data, uint8_t len);
typedef void (*ota_event_cb) (int event, uint8_t *data, int len);

void ota_event_input_cb(int event, uint8_t *event_data, int len);
int ota_client_trans_uart_init(ota_client_event_cb cb);
int ota_client_trans_uart_prepare(slist_t *dev_list);
int ota_client_trans_uart_disconnect();
int ota_client_trans_uart_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack);
int ota_client_trans_uart_reset();
int ota_client_trans_uart_unregister();


#endif
