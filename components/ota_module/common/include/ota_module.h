#ifndef __OTA_MODULE_H
#define __OTA_MODULE_H


typedef enum {
    OTA_BY_GATT = 0x00,
    OTA_BY_UART,
    OTA_BY_HCI_UART,
    OTA_BY_MESH_EXT_1M,
    OTA_BY_MESH_EXT_2M,
    OTA_BY_MESH_LEGACY,
} ota_type_en;


#ifndef CONFIG_ALLOW_OTA_FOR_HIGH_VERSION
#define CONFIG_ALLOW_OTA_FOR_HIGH_VERSION 1
#endif




#endif
