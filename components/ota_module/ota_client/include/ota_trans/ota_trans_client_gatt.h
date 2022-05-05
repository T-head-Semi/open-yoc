/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _OTA_TRANS_CLIENT_GATT_H
#define _OTA_TRANS_CLIENT_GATT_H


int ais_ota_trans_client_gatt_init(void         (*cb)(uint8_t, void*, uint16_t )      );
int ais_ota_trans_client_gatt_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack);
int ais_ota_trans_client_gatt_disconnect();

#endif
