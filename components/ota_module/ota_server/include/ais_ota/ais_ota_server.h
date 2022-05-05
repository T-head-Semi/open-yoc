#ifndef __AIS_OTA_SERVER_H__
#define __AIS_OTA_SERVER_H__

#include "ais_ota.h"
#include "ota_server.h"
#include "ota_process.h"

#ifndef AIS_GATT_WORK_ENVIRONMENT
#define AIS_GATT_WORK_ENVIRONMENT MESH_MODE
#endif



typedef enum {
    AIS_OTA_SERVER_STATE_IDLE = 0x00,
    AIS_OTA_SERVER_STATE_CHAN_READY,
    AIS_OTA_SERVER_STATE_CHAN_CREATE_CONN_FAIL,
    AIS_OTA_SERVER_STATE_CHAN_CONN,
    AIS_OTA_SERVER_STATE_CHAN_DISCONN,
    AIS_OTA_SERVER_STATE_ONGOING,
} ais_ota_server_state;


typedef enum {
    AIS_OTA_INVALID_VERSION,
    AIS_OTA_DEV_REFUSE,
} ais_ota_faild;


int ais_ota_server_init(uint8_t channel);
int ais_ota_server_prepare(uint16_t index,upgrade_firmware   *firmware_info);
int ais_ota_server_ota_start();
int ais_ota_server_send_start();
int ais_ota_server_ota_stop();
int ais_ota_server_reset();
int ais_ota_server_channel_disconnect();


#endif
