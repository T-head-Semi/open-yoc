/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _APP_GATEWAY_OTA_H_
#define _APP_GATEWAY_OTA_H_

#ifndef CONFIG_OTA_GATEWAY_IMG_SIZE
#define CONFIG_OTA_GATEWAY_IMG_SIZE 800*1024
#endif



enum {
	OTA_FOR_GATEWAY  = 0x0,
	OTA_FOR_BLE_NODE = 0x01,
};

typedef void   *gateway_ota_image_t;

int gateway_ota_gateway_upgrade(const void *img_addr, uint32_t img_size, ota_firmware_event_cb firmware_callback);

int gateway_ota_node_upgrade(const void *image, uint32_t size,  device_info *node_list, uint32_t list_size, ota_firmware_event_cb firmware_callback);
int gateway_ota_node_upgrade_with_channel(const void * image, uint32_t size, uint8_t protocol, uint8_t chanel, device_info * node_list, uint32_t list_size, ota_firmware_event_cb firmware_callback);

gateway_ota_image_t gataway_ota_image_alloc(gateway_ota_image_t image, uint32_t size);

void gataway_ota_image_free(gateway_ota_image_t image);

int gateway_ota_init();

#endif

