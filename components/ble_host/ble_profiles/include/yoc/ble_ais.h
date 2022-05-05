/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef __BLE_AIS_H__
#define __BLE_AIS_H__

#include <misc/byteorder.h>
#include <bluetooth/gatt.h>
#include <aos/ble.h>



#define AIS_SERVICE_UUID BT_UUID_DECLARE_16(0xFEB3)
#define AIS_READ_UUID BT_UUID_DECLARE_16(0xFED4)
#define AIS_WRITE_UUID BT_UUID_DECLARE_16(0xFED5)
#define AIS_INDICATE_UUID BT_UUID_DECLARE_16(0xFED6)
#define AIS_WRITE_WO_RESP_UUID BT_UUID_DECLARE_16(0xFED7)
#define AIS_NOTIFY_UUID BT_UUID_DECLARE_16(0xFED8)


typedef void (*ais_cb)(uint8_t event, const void *event_data,uint32_t len);

#ifndef _AIS_PDU_T_
#define _AIS_PDU_T_
//AIS PDU FORMAT
typedef struct {
    uint8_t msg_id : 4;
    uint8_t enc : 1;
    uint8_t ver : 3;
    uint8_t cmd;
    uint8_t seq : 4;
    uint8_t total_frame : 4;
    uint8_t payload_len;
} __attribute__((packed)) ais_header_t;


typedef struct {
    ais_header_t header;
    uint8_t payload[16];
} __attribute__((packed)) ais_pdu_t;
#endif

typedef struct _ble_ais_ctx_s {
	uint8_t init_flag;
    struct bt_conn *p_conn;
    ais_cb user_cb;
} ble_ais_ctx_t;


typedef struct {
  dev_addr_t addr;
  uint8_t status;
}connect_status;

#endif
