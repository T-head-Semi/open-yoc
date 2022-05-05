/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <errno.h>
#include <string.h>
#include "yoc/fota.h"
#include <ulog/ulog.h>

#define TAG "fotav"

#ifndef __linux__
#include <yoc/partition.h>
#include <verify.h>
#include <verify_wrapper.h>

#define DUMP_DATA_EN 0

typedef struct {
    uint32_t magic;
    uint8_t ver;
    uint8_t image_sum;
    uint8_t digest_type;
    uint8_t signature_type;
    uint32_t mnft_off;
    uint8_t head_len;
    uint8_t pad_type;
    uint8_t rsvd[2];
} fota_head_info_t;

#if DUMP_DATA_EN
static void dump_data(uint8_t *data, int32_t len)
{
    int32_t i;

    for (i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("\n");
        }

        printf("%02x ", data[i]);
    }

    printf("\n");
}
#endif

#if defined(CONFIG_FOTA_DATA_IN_RAM) && CONFIG_FOTA_DATA_IN_RAM > 0
/**
 * @brief  获取存储fota数据的ram地址
 * @return address
 */
__attribute__((weak)) unsigned long fota_data_address_get(void)
{
    return 0;
}
#endif

__attribute__((weak)) int fota_data_verify(void)
{
#define FOTA_DATA_MAGIC 0x45474d49
#define BUF_SIZE 512
    int ret;
    uint8_t *buffer;
    uint8_t *hash_out;
    uint32_t olen;
    int fota_data_offset, image_size;
    int signature_len, hash_len;
    int signature_offset, hash_offset;
    digest_sch_e digest_type;
    signature_sch_e sign_type;
    partition_t partition;
    partition_info_t *partition_info;
    fota_head_info_t *head;
    unsigned long data_address_start;

    LOGD(TAG, "start fota verify...");
    ret = 0;
    buffer = NULL;
    hash_out = NULL;
#if CONFIG_FOTA_DATA_IN_RAM > 0
    (void)partition;
    (void)partition_info;
    data_address_start = fota_data_address_get();
    if (data_address_start == 0) {
        LOGE(TAG, "fota data address[0x%x] error.", data_address_start);
        return -1;
    }
    head = (fota_head_info_t *)data_address_start;
    fota_data_offset = 0;
    buffer = aos_malloc(BUF_SIZE);
#else
    (void)data_address_start;
    partition = partition_open("misc");
    if (partition < 0) {
        LOGE(TAG, "flash open e.");
        return -1;
    }
    partition_info = hal_flash_get_info(partition);
    if (partition_info == NULL) {
        ret = -EIO;
        goto out;
    }
    fota_data_offset = partition_info->sector_size << 1;
    buffer = aos_malloc(BUF_SIZE);
    if (buffer == NULL) {
        ret = -ENOMEM;
        goto out;
    }
    if (partition_read(partition, fota_data_offset, buffer, BUF_SIZE) < 0) {
        ret = -EIO;
        goto out;
    }
    head = (fota_head_info_t *)buffer;
#endif
    if (head->magic != FOTA_DATA_MAGIC) {
        LOGE(TAG, "app fota data magic e");
        ret = -1;
        goto out;
    }
    image_size = head->mnft_off;
    digest_type = head->digest_type;
    sign_type = head->signature_type;
    hash_len = get_length_with_digest_type(digest_type);
    signature_len = get_length_with_signature_type(sign_type);
    signature_offset = image_size + fota_data_offset;
    hash_offset = signature_offset + 256;

    LOGD(TAG, "image_size:%d", image_size);
    LOGD(TAG, "digest_type:%d", digest_type);
    LOGD(TAG, "sign_type:%d", sign_type);
    LOGD(TAG, "hash_len:%d", hash_len);
    LOGD(TAG, "signature_len:%d", signature_len);
    LOGD(TAG, "signature_offset:%d", signature_offset);
    LOGD(TAG, "hash_offset:%d", hash_offset);

    hash_out = aos_malloc(hash_len);
    if (hash_out == NULL) {
        ret = -ENOMEM;
        goto out;
    }
#if CONFIG_FOTA_DATA_IN_RAM > 0
    ret = hash_calc_start(digest_type, (const uint8_t *)(fota_data_offset + data_address_start), image_size, hash_out, &olen, 1);
    if (ret != 0) {
        LOGE(TAG, "hash calc failed.");
        goto out;
    }
    memcpy(buffer, (void *)(hash_offset + data_address_start), hash_len);
#else
    ret = hash_calc_start(digest_type, (const uint8_t *)((long)(fota_data_offset + partition_info->base_addr + partition_info->start_addr)), image_size, hash_out, &olen, 0);
    if (ret != 0) {
        LOGE(TAG, "hash calc failed.");
        goto out;
    }
    if (partition_read(partition, hash_offset, buffer, hash_len) < 0) {
        ret = -EIO;
        goto out;
    }
#endif

#if CONFIG_FOTA_IMG_AUTHENTICITY_NOT_CHECK == 0
    // TODO: not support yet
    uint8_t *key;
    int key_len = 0;
    if (partition_read(partition, signature_offset, buffer, signature_len) < 0) {
        ret = -EIO;
        goto out;
    }
#if DUMP_DATA_EN
    dump_data(buffer, signature_len);
#endif
    ret = signature_verify_start(digest_type, sign_type, key, ken_len, hash_out, hash_len, buffer, signature_len);
    if (ret != 0) {
        LOGE(TAG, "signature verify failed.");
        goto out;
    }
    LOGD(TAG, "fota data verify ok...");
#else
    (void)sign_type;
    (void)signature_len;
#if DUMP_DATA_EN
    dump_data(buffer, hash_len);
    dump_data(hash_out, hash_len);
#endif
    if (memcmp(hash_out, buffer, hash_len) != 0) {
        LOGE(TAG, "!!!fota data hash v e..");
        ret = -EIO;
        goto out;
    }
    LOGI(TAG, "###fota data hash v ok.");
#endif /*CONFIG_FOTA_IMG_AUTHENTICITY_NOT_CHECK*/

out:
    if (buffer)
        aos_free(buffer);
    if (hash_out)
        aos_free(hash_out);
#if CONFIG_FOTA_DATA_IN_RAM == 0
    partition_close(partition);
#endif
    return ret;
}

#else

__attribute__((weak)) int fota_data_verify(void)
{
    return 0;
}

#endif /*__linux__*/