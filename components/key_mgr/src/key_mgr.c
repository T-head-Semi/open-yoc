/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include "key_mgr.h"
#include "key_mgr_parser.h"
#include "key_mgr_pub_key.h"

uint32_t g_km_init = 0;

const uint8_t g_km_key_e[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
};


/**
  \brief       km initialiez.
*/
uint32_t km_init(void)
{
    uint32_t ret = KM_OK;

    if (!g_km_init) {
#if (CONFIG_TB_KP > 0)
        ret = parser_init();
        if (ret) {
            return ret;
        }
#endif
#ifndef CONFIG_KEY_MGR_NO_PUB_PARTITION
        ret = km_pub_key_init();
        if (ret) {
            return ret;
        }
#endif
        g_km_init = 1;
    }
    return ret;
}

/**
  \brief       km uninitialiez.
*/
void km_uninit(void)
{
}

/**
  \brief       Get key from km
*/
__attribute__((weak)) uint32_t km_get_key(km_key_type_e key_type, key_handle *key, uint32_t *key_size)
{
    uint32_t ret = KM_ERR;
    if (g_km_init && key_type < KEY_ID_USER_DEFINE_BASE) {
        if (key_type == KM_ID_PUBKEY_E) {
            *key      = (key_handle)g_km_key_e;
            *key_size = sizeof(g_km_key_e);
            ret = KM_OK;
        } else {
#if (CONFIG_TB_KP > 0)
            ret = parser_get_key(key_type, key, key_size);
#endif
        }
    } else { /* key_type >= KEY_ID_USER_DEFINE_BASE */
        /* TODO ADD other key here*/
        key_size = 0;
    }

    /* TODO */

    return ret;
}


/**
  \brief       Get public key from km by name
*/
__attribute__((weak)) uint32_t km_get_pub_key_by_name(const char *name, key_handle *key, uint32_t *key_size)
{
  return g_km_init ? km_get_pub_key_with_name(name, key, key_size) : KM_ERR;
}


/**
  \brief       Derive key function.
*/
uint32_t km_derive_key(km_derive_alg_e alg, uint8_t *key, uint32_t key_size, uint8_t *outkey,
                       uint32_t *size)
{
    return KM_OK;
}

/**
  \brief       Get key from km
*/
uint32_t km_import_key(km_key_type_e key_type, uint8_t *key, uint32_t key_size)
{
    return KM_OK;
}

/**
  \brief       Get key from km
*/
uint32_t km_export_key(km_key_type_e key_type, uint8_t *keybuf, uint32_t buf_size,
                       uint32_t *key_size)
{
    return KM_OK;
}

/**
  \brief       Get key from km
*/
uint32_t km_destory_key(km_key_type_e key_type)
{
    return KM_OK;
}

/**
  \brief       Get info from km
*/
uint32_t km_get_info(km_info_type_e info_type, uint8_t *info_buf, uint32_t buf_size,
                     uint32_t *info_size)
{
    return KM_OK;
}
