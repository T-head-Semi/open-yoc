/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <key_mgr_parser.h>
#include "key_mgr_pub_key.h"
#include "key_mgr_log.h"
#include "key_mgr_port.h"
#include "yoc/partition.h"
#if defined(CONFIG_KEY_MGR_KP_PROTECT) && (CONFIG_KEY_MGR_KP_PROTECT > 0)
#include "sec_crypto_aes.h"
#endif

#if (CONFIG_TB_KP > 0)

#if (CONFIG_KEY_FROM_OTP > 0)
#include <tee_addr_map.h>
#define KP_BUFFER_LEN (CONFIG_OTP_BANK_SIZE >> 2)
#else
#define KP_BUFFER_LEN (1024 >> 2)
#endif

#define KP_IDX_MAGIC 0x78446E49 // "InDx"

#include <soc.h>

#ifdef CONFIG_KEY_MGR_KP_IN_FLASH
uint32_t g_parser_buf[CONFIG_KEY_MGR_KP_SIZE / 4];
#else
uint32_t g_parser_buf[KP_BUFFER_LEN];
#endif

#include "key_mgr_efuse.h"

typedef struct {
    uint16_t type : 1;
    uint16_t rsvr : 15;
} parser_key_flag_t;

typedef struct {
    uint16_t          type;
    uint16_t          size;
    parser_key_flag_t flag;
    uint16_t          rsvr;
    uint32_t          offset;
    uint32_t          rsvr2;
} parser_key_info_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t rsv;
    uint16_t keynum;
    uint16_t digest_type;
    uint32_t total_size;
} parser_head_t;

typedef struct {
    parser_head_t     head;
    parser_key_info_t key[0];
} parser_t;

#if defined(CONFIG_KEY_MGR_KP_PROTECT) && (CONFIG_KEY_MGR_KP_PROTECT > 0)
static uint8_t g_km_protk[32] = {
    0xdf, 0xb6, 0xc4, 0x60, 0x0a, 0x28, 0xe3, 0x72, 0xad, 0xbe, 0xbb, 0xfc, 0x7b, 0xe4, 0xf6, 0xbd,
    0xb9, 0x45, 0x1c, 0x0c, 0xff, 0xb2, 0x4f, 0x8e, 0xdd, 0x58, 0x36, 0xd4, 0x45, 0x2d, 0xfc, 0xd3,
};
#endif

#if (CONFIG_KEY_FROM_OTP > 0)
__attribute__((weak)) uint32_t parser_addr(void)
{
    return CONFIG_OTP_BASE_ADDR;
}

__attribute__((weak)) uint32_t parser_otp_block_size(void)
{
    return CONFIG_OTP_BANK_SIZE;
}
#endif
__attribute__((weak)) uint8_t parser_magic_number_check(uint32_t magic_number)
{
    return (KP_IDX_MAGIC == magic_number);
}

static int parser_check(parser_t *kp)
{
    if (!kp) {
        return KP_ERR_NULL;
    }

    if (0 == parser_magic_number_check(kp->head.magic)) {
        return KP_ERR_MAGIC_NUM;
    }

    if (0x03 != kp->head.version) {
        return KP_ERR_VERSION;
    }

    return KP_OK;
}
static int parser_read_kp(uint8_t *buf, uint32_t len, uint32_t *olen)
{
    int ret = 0;
    if (olen) {
        *olen = 0;
    }
#if (CONFIG_KEY_FROM_OTP > 0)
    ret = efusec_read_bytes(parser_addr(), buf, len);
    if (ret <= 0) {
        return KM_ERR;
    }

    if (ret > parser_otp_block_size()) {
        KM_LOGE("efuse read size too long, %d\n", ret);
        return KM_ERR_TOO_LONG;
    }
#else
    mtb_partition_info_t part_info;
    uint32_t             part_start, image_size;

    part_start = INVALID_ADDR;
    image_size = INVALID_ADDR;
    if (!mtb_get_partition_info(MTB_IMAGE_NAME_OTP, &part_info)) {
        part_start = part_info.start_addr;
        image_size = part_info.img_size;
    } else {
        KM_LOGI("mtb cant f `otp`");
        if (!mtb_get_partition_info(MTB_IMAGE_NAME_KP, &part_info)) {
            part_start = part_info.start_addr;
            image_size = part_info.img_size;
        } else {
            KM_LOGI("mtb cant f `kp`");
        }
    }

    if (part_start == INVALID_ADDR || image_size == INVALID_ADDR) {
        return KM_ERR;
    }

    if (image_size > len) {
        KM_LOGE("flash read size too long, %d\n", image_size);
        return KM_ERR_TOO_LONG;
    }

    if (get_data_from_addr(part_start, (uint8_t *)buf, image_size)) {
        return KM_ERR;
    }
    if (olen) {
        *olen = image_size;
    }
#endif
    return ret;
}

static int parser_get_kp_from_flash(uint8_t **buf, uint32_t *kp_size)
{
    int ret = 0;

#ifdef CONFIG_KEY_MGR_KP_FLASH_ADDR
    *buf = (uint8_t *)CONFIG_KEY_MGR_KP_FLASH_ADDR;
#else
    KM_LOGE("addr err\n");
    return KM_ERR;
#endif

#ifdef CONFIG_KEY_MGR_KP_SIZE
    *kp_size = CONFIG_KEY_MGR_KP_SIZE;
#else
    *kp_size = 1024;
#endif

    return ret;
}

uint32_t parser_init(void)
{
    uint32_t kp_size;
    uint8_t *buf = NULL;
#ifndef CONFIG_KEY_MGR_KP_IN_FLASH
    memset(g_parser_buf, 0, sizeof(g_parser_buf));
    if (parser_read_kp((uint8_t *)g_parser_buf, sizeof(g_parser_buf), &kp_size) != KM_OK) {
        KM_LOGE("read kp err\n");
        return KM_ERR;
    }
#else

    if (parser_get_kp_from_flash(&buf, &kp_size) != KM_OK) {
        KM_LOGE("read kp err\n");
        return KM_ERR;
    }
    memcpy(g_parser_buf, buf, kp_size);
#endif

#if defined(CONFIG_KEY_MGR_KP_PROTECT) && (CONFIG_KEY_MGR_KP_PROTECT > 0)
    sc_aes_t aes_hdl;
    uint32_t ret = sc_aes_init(&aes_hdl, 0);
    if (KP_OK != ret) {
        return ret;
    }
#if (CONFIG_KEY_MGR_KP_PROTECT_OPT > 0)
    ret = sc_aes_set_decrypt_key(&aes_hdl, (void *)g_km_protk, SC_AES_KEY_LEN_BITS_128);
#else
    ret = sc_aes_set_decrypt_key(&aes_hdl, (void *)g_km_protk, SC_AES_KEY_LEN_BITS_256);
#endif
    if (KP_OK != ret) {
        return ret;
    }
    ret = sc_aes_ecb_decrypt(&aes_hdl, (void *)g_parser_buf, (void *)g_parser_buf, kp_size);
    if (KP_OK != ret) {
        return ret;
    }
    sc_aes_uninit(&aes_hdl);
#endif
    if (parser_check((parser_t *)g_parser_buf) != KP_OK) {
        KM_LOGE("kp check err\n");
        return KM_ERR;
    }

    return KM_OK;
}

uint32_t parser_get_key(km_key_type_e key_type, key_handle *key, uint32_t *key_size)
{
    parser_t *         kp;
    parser_key_info_t *key_info;
    uint16_t           key_num;
    uint32_t           key_addr = 0;
    int                i, ret;

    KM_LOGD("key type %d, key %x\n", key_type, key);

    if (!key) {
        return KP_ERR_NULL;
    }

    kp  = (parser_t *)g_parser_buf;
    ret = parser_check(kp);

    if (KP_OK != ret) {
        return ret;
    }

    KM_LOGD("kp addr %x\n", kp);

    key_info = kp->key;
    key_num  = kp->head.keynum;

    for (i = 0; i < key_num; i++) {
        if (key_info->type == key_type) {
            break;
        }

        key_info++;
    }

    if (i >= key_num) {
        return KP_ERR_NOT_FOUND;
    }

    if (0 == key_info->size) {
        return KP_ERR_KEY_SIZE;
    }

    if (0x03 != kp->head.version) {
        return KP_ERR_VERSION;
    }

    key_addr = (uint32_t)g_parser_buf + key_info->offset;
    *key     = key_addr;

    KM_LOGD("find key_addr %x, size %d\n", key_addr, key_info->size);

    *key_size = key_info->size;

    return KM_OK;
}
#else

uint32_t parser_init(void)
{
    return KM_ERR;
}

uint32_t parser_get_key(km_key_type_e key_type, key_handle *key, uint32_t *key_size)
{
    return KM_ERR;
}

#endif /* CONFIG_TB_KP */
