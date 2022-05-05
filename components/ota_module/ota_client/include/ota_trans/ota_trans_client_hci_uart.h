/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _OTA_TRANS_CLIENT_HCI_UART_H
#define _OTA_TRANS_CLIENT_HCI_UART_H


int ais_ota_trans_client_hci_uart_init(void         (*cb)(uint8_t, void*, uint16_t )      );
int ais_ota_trans_client_hci_uart_disconnect();
int ais_ota_trans_client_hci_uart_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack);

#endif
