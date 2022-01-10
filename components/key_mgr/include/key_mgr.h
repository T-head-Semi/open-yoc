/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef KM_H
#define KM_H
#include <stdint.h>

#ifndef CONFIG_MAX_PUB_KEY_NUM
#define MAX_PUB_KEY_NUM 10
#else
#define MAX_PUB_KEY_NUM CONFIG_MAX_PUB_KEY_NUM
#endif

#ifndef CONFIG_MAX_PUB_KEY_SIZE
#define MAX_PUB_KEY_SIZE 256
#else
#define MAX_PUB_KEY_SIZE CONFIG_MAX_PUB_KEY_SIZE
#endif

#define DEFAULT_PUBLIC_KEY_NAME_IN_OTP "def_otp"
typedef enum {
    /* kp */
    KEY_ID_MNF_TB_V2_ADDR           = 1,  /* Bootldr Manifest TB V2 store position */
    KEY_ID_JTAG_KEY                 = 2,  /* Use for verify jtag re-enable request */
    KEY_ID_PUBK_TB                  = 3,  /* Use for verify tb or image signatures */
    KEY_ID_CHIPID                   = 4,  /* In new design, it may be ID2ID or CID */
    KEY_ID_ID2PRVK                  = 5,
    KEY_ID_SOCCONFIG                = 6,
    KEY_ID_LPMRECOVERY              = 7,
    KEY_ID_PUBK_TB_MOD              = 8,
    KEY_ID_EMMC_TRUST_SCRAMBLE_KEY  = 9,  /* EMMC security key */
    KEY_ID_EMMC_NTRUST_SCRAMBLE_KEY = 10, /* EMMC non-security key */
    KEY_ID_EFUSE_SECURE_REGIONS     = 11, /* eFuse's secure regions info */
    KEY_ID_MNF_TB_V3_ADDR           = 12, /* Bootldr Manifest TB V3 store position    */
    KEY_ID_MNF_TB_V3_ADDR_BACKUP    = 13, /* Backup-Bootldr Manifest TB V3 store position */
    KEY_ID_IMG_DECRYPT_KEY          = 14,
    KEY_ID_CHALLENGE_KEY            = 15,
    KEY_ID_RPMB_KEY	                = 16,
    KEY_ID_MAC                      = 30,
    KEY_ID_MESSAGE_AUTH             = 31,
    KEY_ID_PRODUCT_KEY              = 32,
    KEY_ID_PRODUCT_SECRET           = 33,
    KEY_ID_PRODUCT_ID               = 34,
    KEY_ID_DEVICE_NAME              = 35,
    KEY_ID_DEVICE_SECRET            = 36,    
    /* yoc */
    KM_ID_PUBKEY_E                  = 49,
    /* user */
    KEY_ID_USER_DEFINE_BASE         = 64,
    /* NOTE add user key type here */

} km_key_type_e;


typedef enum {
    KM_INFO_PRODUCT_NAME = 1,
} km_info_type_e;

typedef uintptr_t key_handle;

typedef enum {
    KM_KDF = 1,
    KM_INVAILD
} km_derive_alg_e;

#define KM_OK 0
#define KM_ERR 0x20000001
#define KM_ERR_NOT_EXIST 0x20000002
#define KM_ERR_VERSION 0x20000003
#define KM_ERR_INVALIED_PARAM 0x20000004
#define KM_ERR_BUF_TOO_SMALL 0x20000005
#define KM_ERR_INVALIED_OPS 0x20000006
#define KM_ERR_NOT_SUPPORT 0x20000007
#define KM_ERR_DRV 0x20000009
#define KM_ERR_TOO_LONG 0x2000000A
#define KM_ERR_PARTITION 0x2000000B

/**
  \brief       km initialiez.
  \return      error code
*/
uint32_t km_init(void);

/**
  \brief       km uninitialiez.
*/
void km_uninit(void);

/**
  \brief       Get key from km
  \param[in]   key_type  key type,see \ref km_key_type_e
  \param[out]  key     key
  \param[out]  key_size     out key size.
  \return      error code
*/
/* TODO weak */
uint32_t km_get_key(km_key_type_e key_type, key_handle *key, uint32_t *key_size);

/**
  \brief       Get key from km by name
  \param[in]   key_type  key type,see \ref km_key_type_e
  \param[out]  key     key
  \param[out]  key_size     out key size.
  \return      error code
*/
uint32_t km_get_pub_key_by_name(const char *name, key_handle *key, uint32_t *key_size);

/**
  \brief       Derive key function.
  \param[in]   key derive alg.
  \param[in]  key     Buffer to get key
  \param[in]   key_size Key buffer max size.
  \param[out]  outkey     Buffer to derive key
  \param[out]  size     derive key size.
  \return      error code
*/
uint32_t km_derive_key(km_derive_alg_e alg, uint8_t *key, uint32_t key_size, uint8_t *outkey, uint32_t *size);


/**
  \brief       Get key from km
  \param[in]   key_type  key type,see \ref km_key_type_e
  \param[in]  key     Buffer to import key
  \param[in]  key_size    key size.
  \return      error code
*/
uint32_t km_import_key(km_key_type_e key_type, uint8_t *key, uint32_t key_size);

/**
  \brief       Get key from km
  \param[in]   key_type  key type,see \ref km_key_type_e
  \param[out]  keybuf     Buffer to export key
  \param[in]  buf_size     Buffer max size.
  \param[out]  key_size     out key size.
  \return      error code
*/
uint32_t km_export_key(km_key_type_e key_type, uint8_t *keybuf, uint32_t buf_size, uint32_t *key_size);

/**
  \brief       Get key from km
  \param[in]   key_type  key type,see \ref km_key_type_e
  \return      error code
*/
uint32_t km_destory_key(km_key_type_e key_type);


/**
  \brief       Get info from km
  \param[in]   info_type  info type
  \param[out]  infobuf     Buffer to get info
  \param[in]  buf_size     Buffer max size.
  \param[out]  info_size     out info size.
  \return      error code
*/
uint32_t km_get_info(km_info_type_e info_type, uint8_t *info_buf, uint32_t buf_size, uint32_t *info_size);

#endif
