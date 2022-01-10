/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <errno.h>
#include <string.h>
#include "mtb.h"
#include "mtb_internal.h"
#include "mtb_log.h"
#include "yoc/partition_flash.h"
#include "yoc/partition.h"
#include "verify.h"
#include "verify_wrapper.h"
#if (CONFIG_PARITION_NO_VERIFY == 0)
#include <key_mgr.h>
#endif

#if !defined(CONFIG_MANTB_VERSION) || (CONFIG_MANTB_VERSION > 3)

int mtbv4_partition_count(void)
{
    imtb_head_v4_t *head;

    head = (imtb_head_v4_t *)mtb_get()->using_addr;
    return head->partition_count;
}

int mtbv4_init(void)
{
    mtb_t *mtb;
    partition_flash_info_t flash_info;
    mtb_partition_info_t part_info;

    mtb = mtb_get();

    void *handle = partition_flash_open(0); // MTB must be in flash0
    partition_flash_info_get(handle, &flash_info);
    partition_flash_close(handle);

    MTB_LOGD("imtb using:0x%lx, valid:0x%lx", mtb->using_addr, mtb->prim_addr);

    if (mtbv4_get_partition_info(MTB_IMAGE_NAME_IMTB, &part_info)) {
        MTB_LOGE("mtb f `imtb` e");
        return -1;
    }
    mtb->one_size = (part_info.end_addr - part_info.start_addr) >> 1;
    if (mtb->prim_addr == part_info.start_addr) {
        mtb->backup_addr = part_info.start_addr + mtb->one_size;
    } else {
        mtb->backup_addr = part_info.start_addr;
    }
    MTB_LOGD("imtb backup:0x%lx", mtb->backup_addr);
    MTB_LOGD("imtb one-size:0x%x", mtb->one_size);
    MTB_LOGD("mtb init over");
    return 0;
}

int mtbv4_get_partition_info(const char *name, mtb_partition_info_t *part_info)
{
    int flashid = 0;
    int i, count;
    imtb_head_v4_t *head;
    imtb_partition_info_v4_t *pp;
    partition_flash_info_t flash_info;

    if (name && part_info) {
        head = (imtb_head_v4_t *)mtb_get()->using_addr;
        pp = (imtb_partition_info_v4_t *)(mtb_get()->using_addr + sizeof(imtb_head_v4_t) + MTB_OS_VERSION_LEN_V4);
        count = head->partition_count;
        for (i = 0; i < count; i++) {
            if (strncmp(pp->name, name, MTB_IMAGE_NAME_SIZE) == 0) {
#if CONFIG_MULTI_FLASH_SUPPORT
                flashid = pp->partition_type.son_type;
#endif
                void *handle = partition_flash_open(flashid);
                partition_flash_info_get(handle, &flash_info);
                partition_flash_close(handle);

                part_info->start_addr = flash_info.start_addr + pp->block_offset * 512;
                part_info->end_addr = part_info->start_addr + pp->block_count * 512;
                part_info->load_addr = pp->load_address;
                memcpy(part_info->pub_key_name, pp->pub_key_name, PUBLIC_KEY_NAME_SIZE);
                memcpy(part_info->name, pp->name, MTB_IMAGE_NAME_SIZE);
                part_info->part_type = pp->partition_type;
                part_info->img_size = pp->img_size;
                return 0;
            }
            pp ++;
        }
    }

    return -EINVAL;
}

int mtbv4_get_partition_info_with_index(int index, mtb_partition_info_t *part_info)
{
    int count;
    int flashid = 0;
    imtb_head_v4_t *head;
    imtb_partition_info_v4_t *pp;
    partition_flash_info_t flash_info;

    if (part_info) {
        head = (imtb_head_v4_t *)mtb_get()->using_addr;
        pp = (imtb_partition_info_v4_t *)(mtb_get()->using_addr + sizeof(imtb_head_v4_t) + MTB_OS_VERSION_LEN_V4);
        count = head->partition_count;
        if (index < count) {
            pp += index;
#if CONFIG_MULTI_FLASH_SUPPORT
            flashid = pp->partition_type.son_type;
#endif
            void *handle = partition_flash_open(flashid);
            partition_flash_info_get(handle, &flash_info);
            partition_flash_close(handle);

            part_info->start_addr = flash_info.start_addr + pp->block_offset * 512;
            part_info->end_addr = part_info->start_addr + pp->block_count * 512;
            part_info->load_addr = pp->load_address;
            memcpy(part_info->pub_key_name, pp->pub_key_name, PUBLIC_KEY_NAME_SIZE);
            memcpy(part_info->name, pp->name, MTB_IMAGE_NAME_SIZE);
            part_info->part_type = pp->partition_type;
            part_info->img_size = pp->img_size;
            return 0;
        }
    }

    return -EINVAL;
}

int mtbv4_crc_check(void)
{
    mtb_t *mtb;
    imtb_head_v4_t *m_head;
    int crc_content_len;
    uint8_t *ocrc;
    uint32_t crc_cla;
    uint32_t crc;

    mtb = mtb_get();
    m_head = (imtb_head_v4_t *)mtb->using_addr;
    crc_content_len = sizeof(imtb_head_v4_t) + MTB_OS_VERSION_LEN_V4 + \
                          m_head->partition_count * sizeof(imtb_partition_info_v4_t);
    ocrc = (uint8_t *)m_head + crc_content_len;
    crc = ocrc[0];

    crc32_calc_start((uint8_t *)m_head, crc_content_len, &crc_cla);

    crc <<= 8;
    crc |= ocrc[1];
    crc <<= 8;
    crc |= ocrc[2];
    crc <<= 8;
    crc |= ocrc[3];

    if (crc != crc_cla) {
        MTB_LOGE("crc verify e[0x%08x,0x%08x]", crc_cla, crc);
        return -1;
    }
    MTB_LOGD("imtb crc verify ok");
    return 0;
}

int mtbv4_verify(void)
{
#if (CONFIG_PARITION_NO_VERIFY == 0)
    MTB_LOGD("mtb verify ok");
#endif
    return 0;
}

#if (CONFIG_PARITION_NO_VERIFY == 0)
int mtbv4_image_verify(const char *name)
{
#define READ_MAX_SIZE 32
#define PART_HEAD_SIZE (sizeof(partition_header_t))
#define PART_TAIL_HEAD_SIZE (sizeof(partition_tail_head_t))
    int        offset, need_verify;
    char *     pub_key_name;
    uint32_t   part_start, olen;
    int        digest_type, sig_type;
    key_handle key_addr;
    uint32_t   key_size;
    uint8_t    buf[READ_MAX_SIZE + PART_HEAD_SIZE];
    uint8_t    old_sha[128];
    uint8_t    old_sig[256];
    uint8_t    part_tail_buf[PART_TAIL_HEAD_SIZE];

    partition_header_t *   p_head;
    partition_tail_head_t *p_tail_head;
    mtb_partition_info_t part_info;

    if (mtbv4_get_partition_info(name, &part_info)) {
        return -1;
    }

    part_start   = part_info.start_addr;
    pub_key_name = part_info.pub_key_name;

    memset(buf, 0, sizeof(buf));
    if (get_data_from_faddr(part_start, buf, sizeof(buf))) {
        return -1;
    }
    offset      = 0;
    need_verify = 0;
    while (offset < READ_MAX_SIZE) {
        p_head = (partition_header_t *)&buf[offset];

        if (memcmp(&p_head->jump, PARTITION_HEAD_MAGIC, 4) &&
            memcmp(&p_head->magic, PARTITION_HEAD_MAGIC, 4) == 0 &&
            memcmp(&p_head->size, PARTITION_HEAD_MAGIC, 4)) {
            need_verify = 1;
            break;
        }
        offset += 4;
    }
    MTB_LOGD("--- %s", name);
    MTB_LOGD("offset:%d, need_verify:%d", offset, need_verify);
    MTB_LOGD("part_start=0x%x", part_start);

    if (need_verify == 1) {
        uint32_t img_content_size = p_head->size;
        uint32_t img_content_addr = part_start + PART_HEAD_SIZE + offset;
        uint32_t part_tail_addr   = img_content_addr + img_content_size;

        MTB_LOGD("p_head->size=0x%x", p_head->size);
        MTB_LOGD("part_tail_addr:0x%x", part_tail_addr);

        if (get_data_from_faddr(part_tail_addr, part_tail_buf, PART_TAIL_HEAD_SIZE)) {
            return -1;
        }
        p_tail_head = (partition_tail_head_t *)part_tail_buf;

        MTB_LOGD("start verify [%s]", name);
        digest_type = p_tail_head->digestType;
        sig_type    = p_tail_head->signatureType;
        if (sig_type == SIGNATURE_RSA_1024 || sig_type == SIGNATURE_RSA_2048) {
            uint8_t sha[64];
            uint32_t hash_addr = part_tail_addr + PART_TAIL_HEAD_SIZE;
            uint32_t sig_addr  = hash_addr;
            int      hash_len  = get_length_with_digest_type(digest_type);
            int      sig_len   = get_length_with_signature_type(sig_type);
            sig_addr += hash_len;
            MTB_LOGD("start check hash");

            hash_calc_start(digest_type, (const uint8_t *)img_content_addr, img_content_size, sha, &olen, 0);
            if (olen != hash_len) {
                MTB_LOGE("sha len calc error:%d,%d", hash_len, olen);
                return -1;
            }

            if (get_data_from_faddr(hash_addr, old_sha, hash_len)) {
                return -1;
            }
#if 0
            printf("old_sha---------------------------------------------------\n");
            dump_data((uint8_t *)old_sha, hash_len);
            printf("calc_sha---------------------------------------------------\n");
            dump_data((uint8_t *)sha, hash_len);
            printf("---------------------------------------------------\n");
#endif
            if (memcmp(old_sha, sha, hash_len)) {
                MTB_LOGE("img hash verify fail [%s]", name);
                return -1;
            }
            if (km_get_pub_key_by_name(pub_key_name, &key_addr, &key_size) == KM_OK) {
                MTB_LOGD("key_addr:0x%x", key_addr);
                if (get_data_from_faddr(sig_addr, old_sig, sig_len)) {
                    return -1;
                }
                MTB_LOGD("sig_addr:0x%x",sig_addr);
#if 0
                printf("key---------------------------------------------------\n");
                dump_data((uint8_t *)key_addr, key_size);
                printf("old_sig---------------------------------------------------\n");
                dump_data((uint8_t *)old_sig, sig_len);
                printf("---------------------------------------------------\n");
#endif
                if (signature_verify_start(digest_type, sig_type, (uint8_t *)(key_addr),
                                   key_size, sha, hash_len, old_sig, sig_len) != 0) {
                    MTB_LOGE("img rsa verify e");
                    return -1;
                }
            } else {
                // FIXME: no pkey
                MTB_LOGW("no pubkey found, %s", pub_key_name);
                MTB_LOGW("no need rsa verify");
                return 0;
            }
            MTB_LOGI("verify [%s] ok", name);
        } else {
            MTB_LOGE("sig type e:%d", p_tail_head->signatureType);
            return -1;
        }
        MTB_LOGD("img verify ok");
    } else {
        MTB_LOGD("img do not need verify.");
    }

    return 0;
}
#endif /* CONFIG_PARITION_NO_VERIFY */

#else

int mtbv4_init(void)
{
    return 0;
}

int mtbv4_verify(void)
{
    return 0;
}

int mtbv4_image_verify(const char *name)
{
    return 0;
}

int mtbv4_get_partition_info(const char *name, mtb_partition_info_t *part_info)
{
    return 0;
}

#endif