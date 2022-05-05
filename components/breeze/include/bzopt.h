/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef BREEZE_OPT_H
#define BREEZE_OPT_H

#include <stdio.h>
//#include "iotx_log.h"

/* update to version 2.0.6 for supporting. */
#define BZ_VERSION                    "2.0.7"

#define BZ_AUTH_TIMEOUT               30000  // not allowed to be 0

#define BZ_TRANSPORT_TIMEOUT          10000

#define BZ_TRANSPORT_VER              0

#define BZ_ATT_HDR_SIZE               3
#define BZ_FRAME_HDR_SIZE             4
#define BZ_ENCRY_BLOCK_LENGTH         16     // aes 128, 16 byte a block
#define BZ_MAX_FRAME_NUMBER           16     // user payload max frame number
#define BZ_GATT_MTU_SIZE_DEFAULT      23     // connect init before mtu exchange, use default mtu size
#define BZ_GATT_MTU_SIZE_MAX          247    // when use long mtu, adjust this max mtu value with BLE stack config
#define BZ_GATT_MTU_SIZE_LIMIT        103

/* Larger MTU size consumes more memory. If out of memory, you need to reduce the MTU size */
#define BZ_FRAME_SIZE_DEFAULT         (BZ_GATT_MTU_SIZE_DEFAULT - BZ_ATT_HDR_SIZE - BZ_FRAME_HDR_SIZE)
#define BZ_FRAME_SIZE_MAX             (BZ_GATT_MTU_SIZE_MAX - BZ_ATT_HDR_SIZE - BZ_FRAME_HDR_SIZE)
#define BZ_FRAME_SIZE_LIMIT           (BZ_GATT_MTU_SIZE_LIMIT - BZ_ATT_HDR_SIZE - BZ_FRAME_HDR_SIZE)

#ifdef EN_LONG_MTU
#define BZ_MAX_PAYLOAD_SIZE           (BZ_FRAME_SIZE_MAX * BZ_MAX_FRAME_NUMBER)
#else
#define BZ_MAX_PAYLOAD_SIZE           (BZ_FRAME_SIZE_DEFAULT * BZ_MAX_FRAME_NUMBER)
#endif

#ifdef  EN_AUTH
#define BZ_ENABLE_AUTH 1
#else
#define BZ_ENABLE_AUTH 0
#endif

#ifdef  CONFIG_AIS_OTA
#define BZ_ENABLE_OTA 1
#else
#define BZ_ENABLE_OTA 0
#endif

#ifdef  EN_COMBO_NET
#define BZ_ENABLE_COMBO_NET 1
#else 
#define BZ_ENABLE_COMBO_NET 0
#endif

#if 0
#define BREEZE_FLOW(...)     log_flow("breeze", __VA_ARGS__)
#define BREEZE_DEBUG(...)    log_debug("breeze", __VA_ARGS__)
#define BREEZE_INFO(...)     log_info("breeze", __VA_ARGS__)
#define BREEZE_WARN(...)     log_warning("breeze", __VA_ARGS__)
#define BREEZE_ERR(...)      log_err("breeze", __VA_ARGS__)
#define BREEZE_FATAL(...)    log_crit("breeze", __VA_ARGS__)
#define BREEZE_TRACE(...)    log_crit("breeze", __VA_ARGS__)
#define BREEZE_EMERG(...)    log_emerg("breeze", __VA_ARGS__)
#else
#define BREEZE_FLOW                     printf
#define BREEZE_DEBUG(format, args...)   printf("[BZ]"format"\n", ##args)
#define BREEZE_INFO(format, args...)    printf("[BZ]"format"\n", ##args)
#define BREEZE_WARN(format, args...)    printf("[BZ]"format"\n", ##args)
#define BREEZE_ERR(format, args...)     printf("[BZ]"format"\n", ##args)
#define BREEZE_FATAL         printf
#define BREEZE_TRACE         printf
#define BREEZE_EMERG         printf

#endif
//#define BZ_VERBOSE_DEBUG
#if defined(BZ_VERBOSE_DEBUG)
#define BREEZE_VERBOSE(...)  log_crit("breeze", __VA_ARGS__)
#else
#define BREEZE_VERBOSE(...)
#endif

#if defined(BLE_4_0)
#define BZ_BLUETOOTH_VER 0x00
#define BZ_MAX_SUPPORTED_MTU 23
#elif defined(BLE_4_2)
#define BZ_BLUETOOTH_VER 0x01
#define BZ_MAX_SUPPORTED_MTU 247
#elif defined(BLE_5_0)
#define BZ_BLUETOOTH_VER 0x10
#define BZ_MAX_SUPPORTED_MTU 247
#else
#define BZ_BLUETOOTH_VER 0x00
#define BZ_MAX_SUPPORTED_MTU 23
#endif

#endif  // BREEZE_OPT_H
