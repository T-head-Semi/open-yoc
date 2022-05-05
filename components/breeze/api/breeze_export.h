/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef BREEZE_API_EXPORT_H
#define BREEZE_API_EXPORT_H

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "bzopt.h"

#define BD_ADDR_LEN      (6)      /**< Length of Bluetooth Device Address. */
#define STR_MODEL_LEN    (20 + 1) /**< Reserved. */
#define STR_SEC_LEN      (40 + 1) /**< Length of device secret. */
#define STR_PROD_SEC_LEN (32 + 1) /**< Length of product secret. */
#define STR_PROD_KEY_LEN (20 + 1) /**< Length of product key. */
#define STR_DEV_KEY_LEN  (32 + 1) /**< Length of device name */

#ifndef MAX_TOKEN_PARAM_LEN
#define MAX_TOKEN_PARAM_LEN 16
#endif

/***** BLE STATUS ******/
typedef enum {
    CONNECTED,                                // connect with phone success
    DISCONNECTED,                             // lost connection with phone
    AUTHENTICATED,                            // success authentication, security key auth
    TX_DONE,                                  // send user payload data complete
    EVT_USER_BIND,                            // user binded, has AuthCode
    EVT_USER_UNBIND,                          // user unbind, no AuthCode
    EVT_USER_SIGNED,                          // user AuthCode sign pass
    NONE
} breeze_event_t;

typedef struct {
    uint8_t protocol_ver;                     // ble awss protocol version
    char    ssid[32 + 1];                     // ble awss ap ssid
    char    pw[64 * 2 + 1];                   // ap password
    uint8_t bssid[6];
    uint8_t apptoken_len;
    uint8_t apptoken[MAX_TOKEN_PARAM_LEN];
    uint8_t token_type;
    uint8_t region_type;
    int     region_id;
    char    region_mqtturl[128];
    uint8_t rand[3];
} breeze_apinfo_t;

typedef struct {
    uint32_t product_id;
    char *product_key;
    char *product_secret;
    char *device_name;
    char *device_secret;
    uint8_t *dev_adv_mac;               // mac address filled in breeze adv data(maybe bt addr or wifi mac)
} breeze_dev_info_t;

typedef enum {
    OTA_CMD = 1,
    OTA_EVT,
} breeze_ota_info_type_t;

typedef enum {
    ALI_OTA_ON_AUTH_EVT,
    ALI_OTA_ON_TX_DONE,
    ALI_OTA_ON_DISCONNECTED,
    ALI_OTA_ON_DISCONTINUE_ERR,
} ali_ota_evt_type_re_t;

typedef struct {
    uint8_t  cmd;
    uint8_t  frame;
    uint8_t  data[BZ_MAX_PAYLOAD_SIZE];
    uint16_t len;
} breeze_ota_cmd_t;

typedef struct {
    uint8_t evt;
    uint8_t d;
} breeze_ota_evt_t;

typedef struct {
    breeze_ota_info_type_t type;
    union {
        breeze_ota_cmd_t m_cmd;
        breeze_ota_evt_t m_evt;
    } cmd_evt;
} breeze_otainfo_t;

/**
 * @brief Callback when device status changed.
 *
 * @param[in] status @n Device Status.
 * @return None.
 * @see None.
 * @note This API should be implemented by user, and will be called by SDK
 *       when device statuc changed, e.g. bluetooth connection status change.
 */
typedef void (*dev_status_changed_cb)(breeze_event_t event);

/**
 * @brief Callback when there is device status to set.
 *
 * @param[in] buffer @n The data to be set.
 * @param[in] model @n Length of the data.
 * @return None.
 * @see None.
 * @note This API should be implemented by user and will be called by SDK.
 */
typedef void (*set_dev_status_cb)(uint8_t *buffer, uint32_t length);

/**
 * @brief Callback when there is device status to get.
 *
 * @param[out] buffer @n The data of device status.
 * @param[out] model @n Length of the data.
 * @return None.
 * @see None.
 * @note This API should be implemented by user and will be called by SDK.
 */
typedef void (*get_dev_status_cb)(uint8_t *buffer, uint32_t length);

/**
 * @brief Callback when there is AWSS info to get.
 *
 * @param[out] buffer @n The data struct of AP info.
 * @return None.
 * @see None.
 * @note This API should be implemented by user and will be called by SDK.
 */
typedef void (*apinfo_ready_cb)(breeze_apinfo_t *ap);

/**
 * @brief Callback when device receive ota releated event.
 *
 * @param[out] ota_cmd @n ota cmd, e.g. 0x20, 0x22, 0x24, 0x28. pls refer to spec. 
 * @param[out] num_frame @n frame number of ota data.
 * @param[out] buffer @n The data of device status.
 * @param[out] lenght @n Length of the data.
 * @return None.
 * @see None.
 * @note This API should be implemented by user and will be called by SDK.
 */

typedef void (*ota_dev_cb)(breeze_otainfo_t *otainfo);

/**
 * This structure includes the information which is 
 * required to initialize the SDK.
 */
struct device_config
{
    uint8_t         bd_addr[BD_ADDR_LEN];
    uint8_t         bd_adv_addr[BD_ADDR_LEN];        // mac address filled in breeze adv data(maybe bt addr or wifi mac)
    char            model[STR_MODEL_LEN];
    uint32_t        product_id;
    char            product_key[STR_PROD_KEY_LEN];
    uint8_t         product_key_len;
    char            product_secret[STR_PROD_SEC_LEN];
    uint8_t         product_secret_len;
    char            device_name[STR_DEV_KEY_LEN];
    uint8_t         device_key_len;
    char            device_secret[STR_SEC_LEN];
    uint8_t         device_secret_len;
    dev_status_changed_cb status_changed_cb;
    set_dev_status_cb     set_cb;
    get_dev_status_cb     get_cb;
    apinfo_ready_cb       apinfo_cb;
    ota_dev_cb            ota_cb;
};

/**
 * @brief Start breeze SDK services.
 *
 * @param[in] dev_info @n Device information
 * @return result 0:success  -1 failed.
 * @see None.
 * @note This API is called by user to initialize and start breeze services.
 */
int breeze_start(struct device_config *dev_conf);

/**
 * @brief Stop breeze services.
 * @return result 0:success  -1 failed.
 * @see None.
 * @note This API is called by user to stop the breeze services.
 */
int breeze_end(void);

/**
 * @brief Initialize breeze awss module.
 *
 * @param[in] cb    The callback to be called by breeze SDK when AP info ready.
 * @param[in] info  The device information required by breeze SDK.
 * @return None.
 * @see None.
 */
void breeze_awss_init(breeze_dev_info_t *info, 
                      dev_status_changed_cb status_change_cb,
                      set_dev_status_cb set_cb,
                      get_dev_status_cb get_cb,
                      apinfo_ready_cb apinfo_rx_cb,
                      ota_dev_cb ota_cb);

/**
 * @brief Start breeze awss process.
 *
 * @param None.
 * @return None.
 * @see None.
 *
 * @note When this API is called, do not call breeze_start anymore.
 */
void breeze_awss_start();

/**
 * @brief Stop breeze, include ble-awss, ble-breeze and ble-stack.
 *
 * @param None.
 * @return None.
 * @see None.
 *
 * @note When this API is called, do not call breeze anymore.
 */
void breeze_awss_stop();

/**
 * @brief Post device status.
 *
 * @param[in] buffer @n Data to post.
 * @param[in] model @n Length of the data.
 * @return result 0: success; others:err code.
 * @see None.
 * @note This API can be used to update date to server, in non-blocked way.
 *       This API uses ble indicate way to send the data.
 */
uint32_t breeze_post(uint8_t *buffer, uint32_t length);

/**
 * @brief Post device status, in a fast way.
 *
 * @param[in] buffer @n Data to post.
 * @param[in] model @n Length of the data.
 * @return result 0: success; others:err code.
 * @see None.
 * @note This API is similiar with breeze_post. The difference is that
 *       ble notify way is used to post the data.
 */
uint32_t breeze_post_fast(uint8_t *buffer, uint32_t length);

/**
 * @brief Post device status with cmd.
 *
 * @param[in] cmd @n cmda to post.0:default, other:for internal use
 * @param[in] buffer @n Data to post.
 * @param[in] model @n Length of the data.
 * @return result 0: success; others:err code.
 * @see None.
 * @note This API can be used to update date to server, in non-blocked way.
 *       This API uses ble indicate way to send the data.
 */
uint32_t breeze_post_ext(uint8_t cmd, uint8_t *buffer, uint32_t length);

/**
 * @brief Post device status with cmd.
 *
 * @param[in] cmd @n cmda to post.0:default, other:for internal use
 * @param[in] buffer @n Data to post.
 * @param[in] model @n Length of the data.
 * @return result 0: success; others:err code.
 * @see None.
 * @note This API uses ble notification way to send the data.
 */
uint32_t breeze_post_ext_fast(uint8_t cmd, uint8_t *buffer, uint32_t length);

/**
 * @brief Append user specific data to the tail of the breeze adv data.
 *
 * @param[in] data @n Data to append.
 * @param[in] len @n Data length.
 * @return None.
 * @see None.
 * @note User can call this API if additional adv data is needed.
 *       Breeze SDK has its own adv data and format, find more details
 *       in Breeze spec.
 */
void breeze_append_adv_data(uint8_t *data, uint32_t len);

/**
 * @brief Restart BLE advertisement. This API will stop and then start the adv.
 *
 * @param None.
 * @return None.
 * @see None.
 * @note User can call this API if he/she wants to update the adv
 *       content from time to time.
 */
void breeze_restart_advertising();

/**
 * @brief Start BLE advertisement. This API will start the adv.
 *
 * @param[in] awss_flag @n if combo, awss need or not.
 * @param[in] bind_state @n device bind state.
 * @return result 0-success, <0-fail.
 * @see None.
 * @note User can call this API if he/she wants to start the adv
 */
int breeze_start_advertising(uint8_t bind_state, uint8_t awss_flag);

/**
 * @brief Stop BLE advertisement. This API will stop the adv.
 *
 * @param None.
 * @return result 0-success, <0-fail.
 * @see None.
 * @note User can call this API if he/she wants to stop the adv
 */
int breeze_stop_advertising(void);

/**
 * @brief get breeze device's bind state.
 *
 * @param None.
 * @return bind_state 0-not bind, 1-binded.
 * @see None.
 */
uint8_t breeze_get_bind_state(void);

/**
 * @brief clear breeze device's bind state.
 *
 * @param None.
 * @return result 0-success, <0-fail.
 * @see None.
 */
int breeze_clear_bind_info(void);

/**
 * @brief disconnect the peer connected device.
 *
 * @param None.
 * @return None.
 * @see None.
 */
void breeze_disconnect_ble(void);

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif // BREEZE_API_EXPORT_H
