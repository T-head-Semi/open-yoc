#ifndef __BLE_OTA_H__
#define __BLE_OTA_H__

#include "stdint.h"

enum {
    AIS_STATE_DISCON,
    AIS_STATE_CONNECT,
    AIS_STATE_AUTH,
    AIS_STATE_IDLE,
    AIS_STATE_OTA,
    AIS_STATE_REBOOT,
    AIS_STATE_WAITREBOOT,
};


enum {
    OTA_FLAG_SILENT = 0x02,
    AIS_RESP_ERR =    0x0F,
    AIS_SCRT_RANDOM = 0x10,
    AIS_SCRT_CIPHER = 0x11,
    AIS_SCRT_RESULT = 0x12,
    AIS_SCRT_ACK =    0x13,
    AIS_LINK_STATUS = 0x14,
    AIS_LINK_ACK =    0x15,
    AIS_OTA_VER_REQ = 0x20,      //APP request get version info
    AIS_OTA_VER_RESP = 0x21,     //Response the version info request
    AIS_OTA_FIRMWARE_REQ = 0x22, //APP push firmware info
    AIS_OTA_UPD_RESP = 0x23,     //Response to APP which can update or not and have received firmware size
    AIS_OTA_STATUS = 0x24,       //Response to APP that last frame number and have received firmware size
    AIS_OTA_CHECK_REQ = 0x25,    //APP download finish,then device check firmware
    AIS_OTA_CHECK_RESP = 0x26,   //Response to APP the result of firmeware check
    AIS_OTA_DATA = 0x2f,          //This is ota firmware data
};

#ifndef _AIS_PDU_T_
#define _AIS_PDU_T_
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

typedef struct {
    uint8_t image_type;
} __attribute__((packed)) ais_ota_ver_req_t;

typedef struct {
    uint8_t image_type;
    uint32_t ver;
} __attribute__((packed)) ais_ota_ver_resp_t;

typedef struct {
    uint8_t image_type;
    uint32_t ver;
    uint32_t fw_size;
    uint16_t crc16;
    uint8_t ota_flag;
} __attribute__((packed)) ais_ota_upd_req_t;

typedef struct {
    uint8_t state;
    uint32_t rx_size;
    uint8_t total_frame;
} __attribute__((packed)) ais_ota_upd_resp_t;

typedef struct {
    uint8_t last_seq : 4;
    uint8_t total_frame : 4;
    uint32_t rx_size;
} __attribute__((packed)) ais_ota_status_report_t;

typedef struct {
    uint8_t state;
} __attribute__((packed)) ais_ota_check_req_t;

typedef struct {
    uint8_t state;
} __attribute__((packed)) ais_ota_check_resp_t;


#endif
