/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc/partition.h>
#include <yoc/partition_flash.h>
#include <errno.h>
#include <string.h>
#include <drv/sasc.h>
#include "mtb_internal.h"
#include "verify.h"
#ifdef CONFIG_TEE_CA
#include <drv/tee.h>
#endif

#ifndef SHOW_PART_INFO_EN
#define SHOW_PART_INFO_EN 0
#endif

#define MAX_FLASH_NUM CONFIG_FLASH_NUM

static struct {
    partition_info_t *scn_list;
    uint8_t num;
} g_partion_array;
static uint32_t g_scn_buf[sizeof(partition_info_t) * CONFIG_MAX_PARTITION_NUM / 4];

extern void partition_flash_register_default(void);

int partition_init(void)
{
#define BUF_SIZE (sizeof(sys_partition_info_t) * CONFIG_MAX_PARTITION_NUM)
    int i, ret, num, flash_idx = 0, size;
    uint8_t buf[BUF_SIZE];
    sys_partition_info_t *part_info;
    partition_flash_info_t cur_flash_info = {0};
    void *temp_flash_dev;
    static int g_part_init_ok = 0;

    if (g_part_init_ok == 1) {
        return g_partion_array.num;
    }
    partition_flash_register_default();

#ifdef CONFIG_TEE_CA
    ret = csi_tee_get_sys_partition(buf, (uint32_t *)&size);
#else
    ret = mtb_init();
    if (ret == 0)
        ret = get_sys_partition(buf, (uint32_t *)&size);
#endif
    if (ret < 0 || size > BUF_SIZE) {
        return -1;
    }

    part_info = (sys_partition_info_t *)buf;
    num = size / sizeof(sys_partition_info_t);
    g_partion_array.scn_list = (partition_info_t *)g_scn_buf;

    while(flash_idx < MAX_FLASH_NUM) {
        memset(&cur_flash_info, 0, sizeof(partition_flash_info_t));
        temp_flash_dev = partition_flash_open(flash_idx);
        partition_flash_info_get(temp_flash_dev, &cur_flash_info);
        if (cur_flash_info.sector_count > 0) {
#if SHOW_PART_INFO_EN
            printf("#############################[flash_idx:%d, part_num:%d]\n", flash_idx, num);
#endif
            for (i = 0; i < num; i++) {
                if ((cur_flash_info.start_addr <= part_info[i].part_addr) &&
                    ((part_info[i].part_addr + part_info[i].part_size) <= (cur_flash_info.start_addr + cur_flash_info.sector_size * cur_flash_info.sector_count))) {
                    partition_info_t *scn = &g_partion_array.scn_list[i];
                    if (scn->sector_size == 0) {
                        g_partion_array.num ++;
                    }
                    memcpy(scn->description, part_info[i].image_name, MTB_IMAGE_NAME_SIZE);
                    scn->base_addr = cur_flash_info.start_addr;
                    scn->start_addr = part_info[i].part_addr - cur_flash_info.start_addr;
                    scn->length = part_info[i].part_size;
                    scn->sector_size = cur_flash_info.sector_size;
                    scn->idx = flash_idx;
#if !defined(CONFIG_MANTB_VERSION) || (CONFIG_MANTB_VERSION > 3)
                    scn->load_addr = part_info[i].load_addr;
                    scn->image_size = part_info[i].image_size;
#endif
#if CONFIG_MULTI_FLASH_SUPPORT
                    scn->type = part_info[i].type;
                    scn->idx = scn->type.son_type;
#endif
#if SHOW_PART_INFO_EN
                    printf("------------------>%s\n", scn->description);
                    printf("scn->base_addr:0x%x\n", scn->base_addr);
                    printf("scn->start_addr:0x%x\n", scn->start_addr);
                    printf("scn->length:0x%x\n", scn->length);
                    printf("scn->sector_size:0x%x\n", scn->sector_size);
                    #if !defined(CONFIG_MANTB_VERSION) || (CONFIG_MANTB_VERSION > 3)
                    printf("scn->load_addr:0x%x\n", scn->load_addr);
                    printf("scn->image_size:0x%x\n", scn->image_size);
                    #endif
                    #if CONFIG_MULTI_FLASH_SUPPORT
                    printf("scn->type:0x%x\n", scn->type);
                    printf("scn->idx:0x%x\n", scn->idx);
                    #endif
#endif
                    if(!(scn->length && scn->sector_size && strlen(scn->description) > 0)) {
                        return -1;
                    }
                }
            }
        }
        partition_flash_close(temp_flash_dev);
        flash_idx++;
    }

    g_part_init_ok = 1;
#if SHOW_PART_INFO_EN
    printf("g_partion_array.num:%d\n", g_partion_array.num);
#endif
    return g_partion_array.num;
}

partition_info_t *partition_info_get(partition_t partition)
{
#if defined(CONFIG_KERNEL_NONE)
    if (partition >= 0 && partition < g_partion_array.num)
#else
    if (partition >= 0 && partition < g_partion_array.num && g_partion_array.scn_list[partition].flash_dev)
#endif
        return &g_partion_array.scn_list[partition];

    return NULL;
}

partition_t partition_open(const char *name)
{
    int len;

    if (name == NULL) {
        return -EINVAL;
    }
    len = strlen(name);
    len = len > MTB_IMAGE_NAME_SIZE ? MTB_IMAGE_NAME_SIZE : len;    
    for (int i = 0; i < g_partion_array.num; i++) {
        if (memcmp(name, g_partion_array.scn_list[i].description, len) == 0) {
            if (g_partion_array.scn_list[i].flash_dev == NULL) {
                void *flash_dev = partition_flash_open(g_partion_array.scn_list[i].idx);
                g_partion_array.scn_list[i].flash_dev = flash_dev;
            }
            return i;
        }
    }

    return -EINVAL;
}

void partition_close(partition_t partition)
{
    partition_info_t *node = hal_flash_get_info(partition);
    if (node) {
        partition_flash_close(node->flash_dev);
        node->flash_dev = NULL;
    }
}

int partition_read(partition_t partition, off_t off_set, void *data, size_t size)
{
    if (off_set < 0 || data == NULL) {
        return -EINVAL;
    }
    partition_info_t *node = hal_flash_get_info(partition);
    if (node != NULL && off_set + size <= node->length) {
        return partition_flash_read(node->flash_dev, node->base_addr + node->start_addr + off_set, data, size);
    }

    return -EINVAL;
}

int partition_write_size = 0; // for kv test
int partition_write(partition_t partition, off_t off_set, void *data, size_t size)
{
    if (off_set < 0 || data == NULL) {
        return -EINVAL;
    }
    partition_write_size += size;
    partition_info_t *node = hal_flash_get_info(partition);
    if (node != NULL && off_set >= 0 && off_set + size <= node->length) {
        return partition_flash_write(node->flash_dev, node->base_addr + node->start_addr + off_set, data, size);
    }
    return -EINVAL;
}

int partition_erase_size = 0; // for kv test
int partition_erase(partition_t partition, off_t off_set, uint32_t block_count )
{
    if (off_set < 0) {
        return -EINVAL;
    }
    partition_erase_size += block_count;
    partition_info_t *node = hal_flash_get_info(partition);
    if (node != NULL && off_set / node->sector_size + block_count <= node->length / node->sector_size) {
        return partition_flash_erase(node->flash_dev, node->base_addr + node->start_addr + off_set, block_count * node->sector_size);
    }

    return -EINVAL;
}

int partition_get_digest(partition_t partition, uint8_t *out_hash, uint32_t *out_len)
{
#define READ_SIZE (1024 + 32)
#define TAIL_BUFF_SIZE (sizeof(partition_tail_head_t) + 128)
    bool got_flag;
    uint8_t *buffer;
    int offset;
    uint8_t tail_buf[TAIL_BUFF_SIZE];
    partition_header_t *phead;
    partition_tail_head_t *tail = NULL;

    if (!(out_hash && out_len)) {
        return -EINVAL;
    }
    buffer = malloc(READ_SIZE);
    if (buffer == NULL) {
        return -ENOMEM;
    }
    if (partition_read(partition, 0, buffer, READ_SIZE) != 0) {
        goto fail;
    }
    got_flag = false;
    offset = 0;
    while (offset < READ_SIZE) {
        phead = (partition_header_t *)&buffer[offset];
        if (memcmp((uint8_t *)&phead->magic, PARTITION_HEAD_MAGIC, 4) == 0
            && memcmp((uint8_t *)&phead->size, (const void *)PARTITION_HEAD_MAGIC, 4) != 0) {
            got_flag = true;
            break;
        }
        offset += 1;
    }
    // printf("offset:%d\r\n", offset);
    if (got_flag) {
        uint32_t img_content_size = phead->size;
        uint32_t tail_offset = sizeof(partition_header_t) + img_content_size + offset;
        partition_info_t *node = hal_flash_get_info(partition);
        // printf("img_content_size:%d, node->length: %d, tail_offset:%d\r\n", img_content_size, node->length, tail_offset);
        if (img_content_size > node->length) {
            goto fail;
        }
        tail = (partition_tail_head_t *)tail_buf;
        if (partition_read(partition, tail_offset, (void *)tail, TAIL_BUFF_SIZE) == 0) {
            // printf("tail->digestType:%d\r\n", tail->digestType);
            switch(tail->digestType) {
                case DIGEST_HASH_SHA1:
                    *out_len = 20;
                    memcpy(out_hash, tail->digest_data, 20);
                    break;
                case DIGEST_HASH_MD5:
                    *out_len = 16;
                    memcpy(out_hash, tail->digest_data, 16);
                    break;
                case DIGEST_HASH_SHA224:
                    *out_len = 28;
                    memcpy(out_hash, tail->digest_data, 28);
                    break;
                case DIGEST_HASH_SHA256:
                    *out_len = 32;
                    memcpy(out_hash, tail->digest_data, 32);
                    break;
                case DIGEST_HASH_SHA384:
                    *out_len = 48;
                    memcpy(out_hash, tail->digest_data, 48);
                    break;
                case DIGEST_HASH_SHA512:
                    *out_len = 64;
                    memcpy(out_hash, tail->digest_data, 64);
                    break;
                default:
                    goto fail;
            }
            free(buffer);
            return 0;
        }
    }

fail:
    free(buffer);
    return -1;
}

int partition_verify(partition_t partition)
{
    int ret;
    partition_info_t *info = partition_info_get(partition);

    ret = -1;
    if (info) {
        ret = mtb_image_verify(info->description);
    }

    return ret;
}

int partition_all_verify(void)
{
    if (g_partion_array.scn_list && g_partion_array.num > 0) {
        for (int i = 0; i < g_partion_array.num; i++) {
            void *flash_dev = partition_flash_open(g_partion_array.scn_list[i].idx);
            g_partion_array.scn_list[i].flash_dev = flash_dev;

            if (partition_verify(i) != 0) {
                partition_flash_close(flash_dev);
                g_partion_array.scn_list[i].flash_dev = NULL;
                return -1;
            }
            partition_flash_close(flash_dev);
            g_partion_array.scn_list[i].flash_dev = NULL;

        }
        return 0;
    }
    return -EINVAL;
}

int partition_set_region_safe(partition_t partition)
{
#if (CONFIG_NOT_SUPORRT_SASC == 0)
    partition_info_t *node = partition_info_get(partition);
    if (node != NULL) {
        uint32_t addr = node->start_addr + node->base_addr;
        int size = node->length;
        int sector_size = node->sector_size;
#ifdef CONFIG_CSI_V2
        csi_sasc_attr_t attr;

        attr.super_ap  = SASC_RW;
        attr.user_ap   = SASC_AP_DENY;
        attr.super_di  = SASC_DI;
        attr.user_di   = SASC_DI;
        attr.is_secure = 1;

        return csi_sasc_flash_config(0, addr, (size + sector_size - 1) / sector_size + SASC_FLASH_1S - 1, attr);
#endif
#ifdef CONFIG_CSI_V1
        return drv_sasc_config_region(addr,
                (size + sector_size - 1) / sector_size + SASC_FLASH_1S - 1 ,
                SASC_AP_RW, SASC_AP_DN, SASC_CD_DI, 1);
#endif
    }
#endif /* CONFIG_NOT_SUPORRT_SASC */
    return -EINVAL;
}

#if CONFIG_MULTI_FLASH_SUPPORT
int get_flashid_by_abs_addr(unsigned long address)
{
    int i;
    uint32_t addr;
    partition_info_t *scn;

    i = 0;
    while(i < g_partion_array.num) {
        scn = &g_partion_array.scn_list[i];
        addr = scn->base_addr + scn->start_addr;
        if (address >= addr && address <= addr + scn->length) {
            return scn->idx;
        }
        i++;
    }
    return 0;
}
#endif
