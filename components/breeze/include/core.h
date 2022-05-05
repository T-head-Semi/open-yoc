/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef BZ_CORE_H
#define BZ_CORE_H

#include <stdint.h>

#include "common.h"
#include "ble_service.h"
#include "transport.h"
#include "auth.h"
#include "extcmd.h"
#include "bzopt.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Definition of breeze advertising data
#define BZ_MAX_ADV_DATA_LEN             (16)      // fixed manufacturer advertising data length
#define BZ_MANUFACT_SPEC_TYPE           (0xFF)    // fixed manufacturer adv data type
#define BZ_ALI_COMPANY_ID               (0x01A8)  // company id
// Vendor ID octet
#define BZ_SDK_VER_Pos                  (0)       // Breeze Version bit0-3
#define BZ_SUB_TYPE_Pos                 (4)       // Breeze adv subtype, bit4-7, subtype 0-15
// Breeze subtype definition
#define BZ_SUB_TYPE_BASIC               (8)       // IoT-BLE basic device type
#define BZ_SUB_TYPE_SEC_BEACON          (9)       // IoT-BLE single direction device type
#define BZ_SUB_TYPE_VOICE               (10)      // IoT-BLE voice device type
#define BZ_SUB_TYPE_DATA                (11)      // IoT-BLE data-control device type
#define BZ_SUB_TYPE_BLE_COMBO           (12)      // IoT-BLE wifi+ble combo device type
// Function mack bit def
#define BZ_FMSK_BLUETOOTH_VER_Pos       (0)       // FMSK bt version, bit 0-1
#define BZ_FMSK_OTA_Pos                 (2)       // FMSK OTA flag, bit2, 0-Breeze OTA unsupport, 1-Breeze OTA supported
#define BZ_FMSK_SECURITY_Pos            (3)       // FMSK sec flag, bit3, 0-no security, 1-security supported
#define BZ_FMSK_SECRET_TYPE_Pos         (4)       // FMSK sec type, bit4, mandatory if bit3==1, 0-perproduct, 1-perdevice
#define BZ_FMSK_BIND_STATE_Pos          (5)       // FMSK bind state, bit5, 0-not bind, 1-binded
#define BZ_FMSK_WIFI_CONFIG_Pos         (6)       // FMSK wifi config state, bit6, 0-not config, 1-configed
// Breeze security type def
#define BZ_SEC_TYPE_PRODUCT             (0)       // security type perproduct
#define BZ_SEC_TYPE_DEVICE              (1)       // security type perdevice
// Breeze bind state def
#define BZ_BIND_STATE_UNBIND            (0)       // unbind state
#define BZ_BIND_STATE_BIND              (1)       // bind state

typedef struct {
    ali_event_handler_t event_handler;
    uint8_t adv_data[BZ_MAX_ADV_DATA_LEN];
    uint16_t adv_data_len;
    uint8_t  adv_mac[IOTB_MAC_LEN];        // mac address filled in breeze adv data(maybe bt addr or wifi mac)
    uint32_t product_id;
    uint8_t product_key[BZ_DEV_PRODUCT_KEY_LEN];
    uint8_t product_key_len;
    uint8_t product_secret[BZ_DEV_PRODUCT_SECRET_LEN];
    uint8_t product_secret_len;
    uint8_t device_name[BZ_DEV_MAX_DEVICE_NAME_LEN];
    uint8_t device_name_len;
    uint8_t device_secret[IOTB_DEVICE_SECRET_LEN];
    uint8_t device_secret_len;
    uint8_t admin_checkin;                  // 0-not checkin, 1-checkin
    uint8_t guest_checkin;                  // 0-not checkin, 1-checkin
} core_t;

ret_code_t core_init(ali_init_t const *p_init);
void core_reset(void);
void core_create_bz_adv_data(uint8_t bind_state, uint8_t awss_flag);
ret_code_t core_get_bz_adv_data(uint8_t *p_data, uint16_t *length);
void core_event_notify(uint8_t evt_type, uint8_t *data, uint16_t length);
void core_handle_err(uint8_t src, uint8_t code);

#ifdef __cplusplus
}
#endif

#endif // BZ_CORE_H
