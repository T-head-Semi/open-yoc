/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _GATEWAY_OTA_SERVICE_H
#define _GATEWAY_OTA_SERVICE_H

#include "stdio.h"
#include "stdint.h"
#include "k_types.h"
#include "aos/list.h"
#include "ota_module.h"


#ifndef CONFIG_OTA_SERVER_CMD_LIST_SIZE
#define CONFIG_OTA_SERVER_CMD_LIST_SIZE 20
#endif

#ifndef CONFIG_OTA_SERVER_STACK_SIZE
#define CONFIG_OTA_SERVER_STACK_SIZE (2048)
#endif

#ifndef CONFIG_OTA_SERVER_PRIO
#define CONFIG_OTA_SERVER_PRIO (11)
#endif

#ifndef CONFIG_OTA_PROCESS_STACK_SIZE
#define CONFIG_OTA_PROCESS_STACK_SIZE (4096)
#endif

#ifndef CONFIG_OTA_PROCESS_PRIO
#define CONFIG_OTA_PROCESS_PRIO (12)
#endif



#define DEF_MAX_OTA_IMAGE   20
#define DEF_MAX_OTA_PROCESS_THREAD 1

#define DEF_MAX_CONN_RETRY 15

#ifndef CONFIG_OTA_MODULE_DEFAULT_PROTOCAL
#define CONFIG_OTA_MODULE_DEFAULT_PROTOCAL  AIS_PROTOCOL
#endif

#define OTA_VERSION_UNKNOW  0xFFFFFF

typedef enum {
    FIRMWARE_STATE_IDLE           = 0x00,
    FIRMWARE_STATE_IN_USE         = 0x01,
    FIRMWARE_STATE_END_USE        = 0x02,
    FIRMWARE_STATE_REMOVED        = 0x03,
} ota_firmware_state_en;

typedef struct {
   uint8_t firmware_index;
}ota_firmware_state_data;


typedef struct _mac_t {
    uint8_t type;
    uint8_t val[6];
} mac_t;


typedef struct {
    mac_t addr;
	uint16_t   unicast_addr;
	uint32_t   old_version;
} device_info;


typedef enum {
    OTA_STATE_IDLE    = 0x00,
    OTA_STATE_ONGOING = 0x01,  //rmind the user
    OTA_STATE_LOAD_COMPLETE = 0x02,
    OTA_STATE_SUCCESS = 0x03,  //rmind the user
    OTA_STATE_FAILD   = 0x04,  //rmind the user
} ota_device_state_en;

typedef void (*ota_firmware_event_cb)(ota_firmware_state_en event,void * data);
typedef void (*ota_device_event_cb)(ota_device_state_en event,void * data);

typedef struct {
	uint8_t firmware_index;
    device_info dev_info;
    uint8_t channel;
    uint32_t old_ver;
} ota_state_ongoing;

typedef struct {
	uint8_t firmware_index;
    device_info dev_info;
    uint8_t  channel;
    uint32_t old_ver;
    uint32_t new_ver;
    uint32_t cost_time;
} ota_state_success;

typedef struct {
	uint8_t firmware_index;
    device_info dev_info;
    uint8_t channel;
    uint32_t old_ver;
    uint32_t new_ver;
    uint32_t cost_time;
    uint8_t  reason;
} ota_state_fail;

typedef struct {
	uint8_t firmware_index;
    uint8_t channel;
	slist_t cancel_list_head;
} ota_state_cancel;




typedef enum {
	OTA_SUCCESS                 = 0x00,
    OTA_FAIL_CONN               = 0x01,
    OTA_FAIL_INVAILD_VERSION    = 0x02,
    OTA_FAIL_DEV_REFUSED        = 0x03,
    OTA_FAIL_CRC_ERR            = 0x04,
    OTA_FAIL_REBOOT_ERR_VERSION = 0x05,
    OTA_FAIL_CANCEL             = 0x06,
    OTA_FAIL_DISCONN            = 0x07,
    OTA_FAIL_GET_VERSION        = 0x08,
    OTA_FAIL_INVAILD_MTU        = 0x09,
    OTA_FAIL_READ_FIRMWARE      = 0x0a,
    OTA_FAIL_SEND_FIRMWARE      = 0x0b,
    OTA_FAIL_DEV_FAILD          = 0x0c,
    OTA_FAIL_REPORT_VERSION_TIMEOUT = 0x0d,
    OTA_FAIL_TIMEOUT                = 0x0e,
    OTA_FAIL_FIRMWARE_DEL           = 0x0f,
    OTA_FAIL_SET_TRANS_ID           = 0x10,  //FOR MESH OTA
    OTA_FAIL_ERR_TRANS_INFO         = 0x11,
} ota_fail_reason;



enum {
  DEV_LIST_FOUND_BY_MAC              = 0x00,
  DEV_LIST_FOUND_BY_UNICAST_ADDR     = 0x01,
  DEV_LIST_FOUND_BY_MAC_UNICAST_ADDR = 0x02,
};


typedef  enum {
    IMAGE_POS_FLASH,
    IMAGE_POS_RAM,
} ota_image_pos_en;

typedef enum {
    AIS_PROTOCOL      = 0x00,
    MESH_OTA_PROTOCOL = 0x01,
} protocol_en;

typedef enum {
   VERSION_GET_IDLE         = 0x00,
   VERSION_GET_SEND_FAIL    = 0x01,
   VERSION_GET_SEND_SUCCESS = 0x02,
   VERSION_GET_SUCCESS      = 0x03,
}version_get_state_en;


typedef struct _firmware_info {
    uint8_t* address;
    int      size;
    uint32_t version;
    uint16_t crc16;
    uint8_t  ota_flag;
    uint8_t  image_type;
    uint8_t  image_pos;
    uint8_t  ota_chanel;
	uint8_t  protocol;
	uint16_t multicast_addr;
    ota_firmware_event_cb cb;
} firmware_info;

typedef struct {
    slist_t      list;
    device_info device;
    ota_device_state_en     ota_state;
	uint8_t      cancel_status;
	uint8_t     version_get_status;
    uint8_t     faild_reason;
	uint8_t     conect_faild_count;
    uint32_t     old_version;
    long long    start_time;
    long long    download_finish_time;
} upgrade_device;

typedef struct _upgrade_firmware {
    firmware_info info;
    slist_t dev_list_head;
    slist_t cancel_list_head;
} upgrade_firmware;


typedef struct _ota_firmware {
    upgrade_firmware firmware;
    uint8_t flags; // 0 ok 1 remove
    long long process_idle_time;
} ota_firmware;


struct ota_server_cb {
    ota_device_event_cb   device_event_cb;
    struct ota_server_cb *_next;
};

typedef struct {
    uint8_t  event;
    uint16_t firmware_index;
} ota_server_cmd_message;

static inline char* ota_fail_str(uint8_t fail_reason)
{
    switch(fail_reason) {
	case OTA_SUCCESS:
		return "SUCCESS";
    case OTA_FAIL_CONN:
        return "FAIL_CONN";
    case OTA_FAIL_INVAILD_VERSION:
        return "INVAILD_VERSION";
    case OTA_FAIL_DEV_REFUSED:
        return "DEV_REFUSED";
    case OTA_FAIL_CRC_ERR:
        return "CRC_ERR";
    case OTA_FAIL_REBOOT_ERR_VERSION:
        return "REBOOT_ERR_VERSION";
    case OTA_FAIL_CANCEL:
        return "CANCEL";
    case OTA_FAIL_DISCONN:
        return "DISCONN";
    case OTA_FAIL_GET_VERSION:
        return "GET_VERSION";
    case OTA_FAIL_INVAILD_MTU:
        return "INVAILD_MTU";
    case OTA_FAIL_READ_FIRMWARE:
        return "FAIL_READ_FIRMWARE";
    case OTA_FAIL_SEND_FIRMWARE:
        return "FAIL_SEND_FIRMWARE";
    case OTA_FAIL_DEV_FAILD:
        return "DEV_FAILD";
	case OTA_FAIL_REPORT_VERSION_TIMEOUT:
        return "REPORT_VERSION_TIMEOUT";
	case OTA_FAIL_FIRMWARE_DEL:
        return "FAIL_FIRMWARE_DEL";
	case OTA_FAIL_SET_TRANS_ID:
        return "FAIL_SET_TRANS_ID";
    default:
        return "UNKNOW";
    }
}

int  ota_server_init();
void ota_server_cb_register(struct ota_server_cb *cb);
void ota_server_cb_unregister(struct ota_server_cb *cb);
int  ota_server_upgrade_firmware_add(firmware_info firware);
int  ota_server_upgrade_firmware_rm(uint16_t index);
ota_firmware* ota_server_upgrade_firmware_get(uint16_t index);
int ota_server_upgrade_device_add(uint16_t index,uint16_t device_list_size,device_info* devices_list);
int ota_server_upgrade_start(uint16_t index);
int ota_server_upgrade_device_rm(uint16_t index,uint16_t device_list_size,device_info* devices_list);


int ota_server_upgrade_firmware_read(uint16_t index,uint32_t offset,uint32_t length,uint8_t* buffer);
int dev_list_add_dev(slist_t *list,upgrade_device *dev,uint8_t type);
upgrade_device*  dev_list_rm_dev(slist_t *list,device_info dev_info,uint8_t type);
int dev_list_rm_and_free_dev(slist_t *list_head,device_info dev_info,uint8_t type);
upgrade_device* dev_list_dev_get(slist_t *list);
upgrade_device* dev_list_dev_search(slist_t *list,device_info dev_info, uint8_t type);
void ota_server_device_event_process(ota_device_state_en event, void * data);
int ota_server_cmd_set(ota_server_cmd_message *cmd);

#endif
