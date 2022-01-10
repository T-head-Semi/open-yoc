/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef KEY_MGR_PARSER_H_
#define KEY_MGR_PARSER_H_


#include <stdint.h>
#include <string.h>
#include <key_mgr.h>

#define TRUST_BOOT_PUBKEY_N_SIZE    128
#define TRUST_BOOT_PUBKEY_E_SIZE    128

#define KP_OK             0
#define KP_ERR_MAGIC_NUM -1
#define KP_ERR_NOT_FOUND -2
#define KP_ERR_VERSION   -3
#define KP_ERR_NULL      -4
#define KP_ERR_POS_TYPE  -5
#define KP_ERR_KEY_SIZE  -6
#define KP_ERR_KEY_TYPE  -7



uint32_t parser_init(void);

/**
  \brief       TEE get key from kp
  \param[in]   key_type  key type,see \ref key_type_e
  \param[out]  key     Pointer to key
  \return      return key size if > 0, else error code
*/
uint32_t parser_get_key(km_key_type_e key_type, key_handle *key, uint32_t *key_size);

/**
  \brief       TEE get kp version
  \return      return kp version if > 0, else error code
*/
int parser_version();

/**
  \brief       TEE get manifest address
  \param[out]  addr  manifest address
  \return      return KP_OK if success, else error code
*/
int parser_get_manifest_addr(uint32_t *addr);


#endif
