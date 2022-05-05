/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef BZ_COMMON_H
#define BZ_COMMON_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bzopt.h"

/* Breeze Bluetooth and Device specified definition */
#define IOTB_MAC_LEN                              6
#define BZ_DEV_PRODUCT_KEY_LEN                    11
#define BZ_DEV_PRODUCT_SECRET_LEN                 16
#define BZ_DEV_MAX_DEVICE_NAME_LEN                32
#define IOTB_DEVICE_SECRET_LEN                    32
#define BZ_DEV_RANDOM_LEN                         16

/* Breeze core profile specified definition */
#define BZ_CMD_TYPE_MASK                          0xf0

#define BZ_CMD_REPORT                             0x1          // peer<-dev, no need to ack
#define BZ_CMD_REQUEST                            0x2          // peer->dev, need 0x3 response
#define BZ_CMD_RESPONSE                           0x3          // peer<-dev, ack to 0x2
#define BZ_CMD_INDICATE                           0x4          // peer<-dev, need 0x5 confirm
#define BZ_CMD_CONFIRM                            0x5          // peer->dev, ack to 0x4
#define BZ_CMD_CTRL                               0x6          // peer->dev, no need to ack
#define BZ_CMD_MANU_REQ                           0x7          // peer->dev, need 0x8 Manufacturer Specific Data response
#define BZ_CMD_MANU_RESP                          0x8          // peer<-dev, ack to 0x7
#define BZ_CMD_EXT_DOWN                           0xd          // peer->dev, internal extension request, need ack
#define BZ_CMD_EXT_UP                             0xe          // peer<-dev, internal extension response
#define BZ_CMD_ERR                                0xf          // peer<-dev, report exception

#define BZ_CMD_TYPE_AUTH                          0x10
#define BZ_CMD_AUTH_SET_RAND                      0x10         // peer->dev, auth random
#define BZ_CMD_AUTH_RPT_CIPHER                    0x11         // peer<-dev, auth cipher
#define BZ_CMD_AUTH_RESULT_IND                    0x12         // peer->dev, auth result indicate
#define BZ_CMD_AUTH_RESULT_CFM                    0x13         // peer<-dev, auth result confirm
#define BZ_CMD_AUTH_BIND_IND                      0x14         // peer->dev, bind result indicate
#define BZ_CMD_AUTH_BIND_CFM                      0x15         // peer<-dev, bind result confirm

#define BZ_CMD_TYPE_AUTH                          0x10
#define BZ_CMD_AUTH_RAND                          0x11
#define BZ_CMD_AUTH_REQ                           0x12
#define BZ_CMD_AUTH_RSP                           0x13
#define BZ_CMD_AUTH_CFM                           0x14
#define BZ_CMD_AUTH_KEY                           0x15

#define BZ_CMD_AUTH_REKEY                         0x16
#define BZ_CMD_AUTH_REKEY_RSP                     0x17


#define BZ_CMD_TYPE_OTA                           0x20
#define BZ_CMD_OTA_VER_REQ                        0x20
#define BZ_CMD_OTA_VER_RSP                        0x21
#define BZ_CMD_OTA_REQ                            0x22
#define BZ_CMD_OTA_RSP                            0x23
#define BZ_CMD_OTA_PUB_SIZE                       0x24
#define BZ_CMD_OTA_CHECK_RESULT                   0x25
#define BZ_CMD_OTA_UPDATE_PROCESS                 0x26
#define BZ_CMD_OTA_SIZE                           0x27
#define BZ_CMD_OTA_DONE                           0x28
#define BZ_CMD_OTA_DATA                           0x2f

typedef uint8_t ret_code_t;

/* Error codes internal. */
#define BZ_SUCCESS                                0
#define BZ_EINVALIDPARAM                          1
#define BZ_EDATASIZE                              2
#define BZ_EINVALIDSTATE                          3
#define BZ_EGATTNOTIFY                            4
#define BZ_EGATTINDICATE                          5
#define BZ_ETIMEOUT                               6
#define BZ_EBUSY                                  7
#define BZ_EINVALIDDATA                           8
#define BZ_EINTERNAL                              9
#define BZ_EINVALIDADDR                           10
#define BZ_ENOTSUPPORTED                          11
#define BZ_ENOMEM                                 12
#define BZ_EFORBIDDEN                             13
#define BZ_ENULL                                  14
#define BZ_EINVALIDLEN                            15
#define BZ_EINVALIDTLV                            16

#define BZ_ERR_MASK                               0xf0
#define BZ_TRANS_ERR                              0x10
#define ALI_ERROR_SRC_TRANSPORT_TX_TIMER          0x10
#define ALI_ERROR_SRC_TRANSPORT_RX_TIMER          0x11
#define ALI_ERROR_SRC_TRANSPORT_1ST_FRAME         0x12
#define ALI_ERROR_SRC_TRANSPORT_OTHER_FRAMES      0x13
#define ALI_ERROR_SRC_TRANSPORT_ENCRYPTED         0x14
#define ALI_ERROR_SRC_TRANSPORT_RX_BUFF_SIZE      0x15
#define ALI_ERROR_SRC_TRANSPORT_PKT_CFM_SENT      0x16
#define ALI_ERROR_SRC_TRANSPORT_FW_DATA_DISC      0x17
#define ALI_ERROR_SRC_TRANSPORT_SEND              0x18

#define BZ_AUTH_ERR                               0x20
#define IOTB_ERROR_AUTH_CIPHER_RPT                0x20
#define ALI_ERROR_SRC_AUTH_PROC_TIMER_0           0x21
#define ALI_ERROR_SRC_AUTH_PROC_TIMER_1           0x22
#define ALI_ERROR_SRC_AUTH_PROC_TIMER_2           0x23
#define ALI_ERROR_SRC_AUTH_SVC_ENABLED            0x24
#define ALI_ERROR_SRC_AUTH_SEND_ERROR             0x25
#define ALI_ERROR_SRC_AUTH_SEND_KEY               0x26

#define BZ_EXTCMD_ERR                             0x30
#define ALI_ERROR_SRC_TYPE_EXT                    0x30
#define ALI_ERROR_SRC_EXT_SEND_RSP                0x31

#define BZ_BIND_ERR                               0x40
#define BZ_ERROR_AC_AS_DATA_LEN                   0x40
#define BZ_ERROR_AC_AS_NO_PERMIT                  0x41
#define BZ_ERROR_AC_AS_DELETE                     0x42
#define BZ_ERROR_AC_AS_STORE                      0x43
#define BZ_ERROR_AUTH_DATA                        0x44
#define BZ_ERROR_AUTH_SIGN                        0x45

#define BLE_CONN_HANDLE_INVALID                   0xffff
#define BLE_CONN_HANDLE_MAGIC                     0x1234

/* Breeze sign and kv-key related definition */
#define BZ_CLIENTID_STR                           "clientId"
#define BZ_SEQUENCE_STR                           "sequence"
#define BZ_DEVICE_NAME_STR                        "deviceName"
#define BZ_DEVICE_SECRET_STR                      "deviceSecret"
#define BZ_PRODUCT_KEY_STR                        "productKey"
#define BZ_PRODUCT_SECRET_STR                     "productSecret"
#define BZ_HI_SERVER_STR                          "Hi,Server"
#define BZ_HI_CLIENT_STR                          "Hi,Client"
#define BZ_OK_STR                                 "OK"

#ifdef EN_AUTH_OFFLINE
#define AUTH_KEY_KV_PREFIX                        "AUTH_KEY_PAIRS"
#endif
#define BZ_AUTH_CODE_KV_PREFIX                    "AUTH_AC_AS" // AC_length(1Byte)+AC+AS_length+AS, when no AS, use DS

enum {
    BZ_EVENT_CONNECTED,                  // BLE connect
    BZ_EVENT_DISCONNECTED,               // BLE disconnect
    BZ_EVENT_AUTHENTICATED,              // Authenticated
    BZ_EVENT_TX_DONE,                    // User payload tx done
    BZ_EVENT_RX_INFO,                    // User payload received
    BZ_EVENT_APINFO,                     // Get AP info data, for ble-awss
    BZ_EVENT_AC_AS,                      // Get User bind data, for ble-bind
    BZ_EVENT_AUTH_SIGN,                  // Get User sign data, for ble-user-sign
    BZ_EVENT_ERR_DISCONT,                // OTA transfer discontinue error occur
};

// User bind data operaton result
enum {
    BZ_AC_AS_ADD,
    BZ_AC_AS_UPDATE,
    BZ_AC_AS_DELETE,
};

// User sign data operaton result
enum {
    BZ_AUTH_SIGN_NO_CHECK_PASS,
    BZ_AUTH_SIGN_CHECK_PASS,
};

typedef struct {
    uint8_t *p_data;
    uint16_t length;
} ali_data_t;

typedef struct {
    uint8_t type;
    ali_data_t rx_data;
} ali_event_t;

typedef void (*ali_event_handler_t)(ali_event_t *p_event);

typedef struct {
    ali_event_handler_t event_handler;
    uint32_t   model_id;
    ali_data_t product_key;                // PK 11 to 20 bytes)
    ali_data_t product_secret;             // secret 16 to 40 bytes
    ali_data_t device_name;                // DN 20 to 32 bytes
    ali_data_t device_secret;              // secret 16 to 40 bytes
    uint8_t *adv_mac;                      // mac address filled in breeze adv data(maybe bt addr or wifi mac)
    uint32_t transport_timeout;            // Timeout of Tx/Rx, in number of ms. 0 if not used.
    uint16_t max_mtu;                      // Maximum MTU.
    uint8_t  *user_adv_data;               // User's adv data, if any.
    uint32_t user_adv_len;                 // User's adv data length
} ali_init_t;

typedef struct {
    uint8_t src;
    uint8_t code;
} bz_err_event_t;

typedef uint32_t (*tx_func_t)(uint8_t cmd, uint8_t *p_data, uint16_t length);

#endif  // BZ_COMMON_H
