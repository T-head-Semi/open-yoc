/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _OTA_TRANS_CLIENT_GATT_H
#define _OTA_TRANS_CLIENT_GATT_H


int ota_client_trans_gatt_init(void         (*cb)(uint8_t, const void*, uint8_t )      );
int ota_client_trans_gatt_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack);
int ota_client_trans_gatt_disconnect();

#endif
