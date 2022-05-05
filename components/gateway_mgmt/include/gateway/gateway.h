/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _GATEWAY_H_
#define _GATEWAY_H_

#include <ota_server.h>

#ifndef CONFIG_OTA_GATEWAY_IMG_SIZE
#define CONFIG_OTA_GATEWAY_IMG_SIZE 800*1024
#endif

#define GATEWAY_DEFAULT_MOD_ID          0x1000
#define GATEWAY_DEFAULT_SUB_ADDR        0xC000
#define GATEWAY_LIVING_ROOM_SUB_ADDR    0xC001
#define GATEWAY_BED_ROOM_SUB_ADDR       0xC002
#define GATEWAY_STUDY_ROOM_SUB_ADDR     0xC003
#define GATEWAY_KITCHEN_SUB_ADDR        0xC004
#define GATEWAY_SHOWER_ROOM_SUB_ADDR    0xC005
#define GATEWAY_BEDSIDE_SUB_ADDR        0xC006
#define GATEWAY_UNKNOWN_SUB_ADDR        0x0

#define GATEWAY_ADD_NODE_SUCCESS            0
#define GATEWAY_ADD_NODE_OCC_AUTH_FAIL      1
#define GATEWAY_ADD_NODE_AUTOCONFIG_FAIL    2
#define GATEWAY_ADD_NODE_GET_TRIPPLES_FAIL  3

typedef void   *gateway_ota_image_t;

int gateway_ota_gateway_upgrade(const void *img_addr, uint32_t img_size, ota_firmware_event_cb firmware_callback);

int gateway_ota_node_upgrade(const void *image, uint32_t size,  device_info *node_list, uint32_t list_size, ota_firmware_event_cb firmware_callback);
int gateway_ota_node_upgrade_with_channel(const void * image, uint32_t size, uint8_t protocol, uint8_t chanel, device_info * node_list, uint32_t list_size, ota_firmware_event_cb firmware_callback);

gateway_ota_image_t gataway_ota_image_alloc(gateway_ota_image_t image, uint32_t size);

void gataway_ota_image_free(gateway_ota_image_t image);

int gateway_ota_init();

/* gateway mgmt interface */
int gateway_btmesh_prov_showdev(uint8_t show_dev);
int gateway_btmesh_prov_add_dev(char *dev_addr_str, uint8_t addr_type, 
                                char *uuid_str, uint16_t oob_info, uint8_t bearer);
int gateway_btmesh_prov_add_dev_cb(char *dev_addr, uint8_t rst_code);                                
int gateway_btmesh_subdev_turn_onoff(const char *mac, uint8_t onoff);
int gateway_btmesh_subdev_set_brightness(const char *mac, uint8_t bri);
int gateway_btmesh_subdev_set_cct(const char *mac, uint16_t cct);
int gateway_btmesh_subdev_turn_onoff_by_position(char *device_type, char *position, uint8_t onoff);
int gateway_btmesh_subdev_update_onoff_by_position(char *device_type, char *position, uint8_t onoff);
int gateway_btmesh_subdev_rpt_onoff(const char *mac, uint8_t onoff);
int gateway_btmesh_subdev_rpt_rst(const char *mac, int rst_code);
int gateway_btmesh_prov_showdev_cb(const char *dev_addr_str, const char *uuid_str);
int gateway_btmesh_prov_node_auto_config(uint32_t addr);
int gateway_btmesh_subdev_info_cfg(const char *mac, const char *room_type, const char *device_type);
int gateway_btmesh_prov_del_dev(char *dev_addr_str);
int gateway_btmesh_subdev_sub(char *mac, uint16_t mod_id, uint16_t sub_addr);


#endif

