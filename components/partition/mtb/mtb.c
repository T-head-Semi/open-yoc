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

static mtb_t g_mtb;
#ifdef CONFIG_NON_ADDRESS_FLASH
uint8_t __attribute__((aligned(4))) g_manfest_tb_ram[MAX_MANTB_BYTE_SIZE];
#endif

static int mtb_verify(void);
static int mtb_crc_check(void);

int get_data_from_faddr(unsigned long addr, void *data, size_t data_len)
{
    int ret = 0;
#ifdef CONFIG_NON_ADDRESS_FLASH
    int flashid = 0;

#if CONFIG_MULTI_FLASH_SUPPORT
    flashid = get_flashid_by_abs_addr(addr);
#endif

    void *handle = partition_flash_open(flashid);
    ret = partition_flash_read(handle, addr, data, data_len);
    partition_flash_close(handle);

#else
    memcpy(data, (void *)addr, data_len);
#endif /* CONFIG_NON_ADDRESS_FLASH */

    return ret;
}

static int mtb_find_addr(mtb_t *mtb, unsigned long find_start, unsigned long find_end, uint32_t block_size)
{
    unsigned long flash_addr = 0;
    unsigned long flash_end;

    flash_addr = find_start;
    flash_end = find_end;

    MTB_LOGD("start_addr:0x%08lx", flash_addr);
    MTB_LOGD("block_size:%d", block_size);
    MTB_LOGD("flash_end:0x%lx", flash_end);
#ifdef CONFIG_NON_ADDRESS_FLASH
    mtb_head_t m_buf;
    int bmtb_size = 0;
    int imtb_size = 0;

    while(flash_addr < flash_end) {
        if (!get_data_from_faddr(flash_addr, &m_buf, sizeof(mtb_head_t))) {
            if (m_buf.head.magic == BMTB_MAGIC) {
                MTB_LOGD("find bmtb %d", m_buf.head.version);
                if (m_buf.head.version == 1) {
                    if (flash_addr + m_buf.head.size < flash_end) {
                        mtb_head_t i_buf;
                        if (!get_data_from_faddr(flash_addr + m_buf.head.size, &i_buf,
                                                    sizeof(mtb_head_t))) {
                            if (i_buf.head.magic == MTB_MAGIC) {
                                bmtb_size = m_buf.head.size;
                                mtb->prim_addr = flash_addr;
                                imtb_size = i_buf.head.size;
                                mtb->version = 1;
                                break;
                            }
                        }
                    }
                }
            }
            if (m_buf.head.magic == MTB_MAGIC) {
                MTB_LOGD("find imtb %d", m_buf.head.version);
                mtb->prim_addr = flash_addr;
                mtb->version = m_buf.head.version;
                if (m_buf.head.version < 0x4) {
                    imtb_size = m_buf.head.size;
                } else {
                    imtb_size = m_buf.headv4.size;
                }
                MTB_LOGD("imtb_size:%d", imtb_size);
                break;
            }
        }
        flash_addr += block_size;
    }

    if (flash_addr == flash_end) {
        MTB_LOGE("mtb cant find magic");
        goto fail;
    }

    if (imtb_size > MAX_MANTB_BYTE_SIZE) {
        MTB_LOGE("mtb size overflow");
        goto fail;
    }

    if (get_data_from_faddr(flash_addr + bmtb_size, g_manfest_tb_ram, imtb_size)) {
        goto fail;
    }

    mtb->using_addr = (unsigned long)g_manfest_tb_ram;
    mtb->i_offset = bmtb_size;
#else
    while(flash_addr < flash_end) {
        mhead_tb *phead = (mhead_tb *)flash_addr;
        if (phead->magic == BMTB_MAGIC) {
            MTB_LOGD("find bmtb %d", phead->version);
            // version 1
            if (phead->version == 1) {
                MTB_LOGD("size %d", phead->size);
                if (flash_addr + phead->size < flash_end) {
                    mhead_tb *ihead = (mhead_tb *)(flash_addr + phead->size);
                    if (ihead->magic == MTB_MAGIC) {
                        mtb->prim_addr = flash_addr;
                        mtb->using_addr = flash_addr + phead->size;
                        mtb->version = 1;
                        mtb->i_offset = phead->size;
                        MTB_LOGD("got mtb1");
                        break;
                    }
                }
            }
        }
        if (phead->magic == MTB_MAGIC) {
            mtb->prim_addr = flash_addr;
            mtb->using_addr = flash_addr;
            mtb->version = phead->version;
            mtb->i_offset = 0;
            MTB_LOGD("got mtb%d", mtb->version);
            break;
        }
        flash_addr += block_size;
    }

    if (flash_addr == flash_end) {
        MTB_LOGE("mtb find magic e\n");
        goto fail;
    }
#endif
    MTB_LOGD("mtb find 0x%lx", mtb->prim_addr);
    return 0;
fail:
    MTB_LOGE("mtb find magic failed.");
    return -1;
}

int mtb_init(void)
{
    int ret = -1;
    void *handle;
    uint32_t flash_addr;
    uint32_t flash_end;
    partition_flash_info_t flash_info;

    handle = partition_flash_open(0);   // MTB must be in flash0
    ret = partition_flash_info_get(handle, &flash_info);
    partition_flash_close(handle);

    if (ret < 0) {
        MTB_LOGE("get flash info e.[%d]", ret);
        return -1;
    }

    flash_addr = flash_info.start_addr;
    flash_end = flash_addr + flash_info.sector_size * flash_info.sector_count;
    if (mtb_find_addr(&g_mtb, flash_addr, flash_end, flash_info.sector_size)) {
        MTB_LOGE("mtb find magic error");
        return -1;
    }

    if (g_mtb.version < 4) {
        ret = mtbv_init();
    } else {
        ret = mtbv4_init();
    }
    if (ret == 0) {
        ret = mtb_crc_check();
    }
    return ret;
}

uint32_t mtb_get_addr(uint8_t is_prim)
{
    if (is_prim == 0) {
        return g_mtb.backup_addr;
    }
    return g_mtb.prim_addr;
}

static int mtb_crc_check(void)
{
    int ret = 0;

    if (g_mtb.version < 4) {
        // do nothing
        ret = 0;
    } else {
        ret = mtbv4_crc_check();
    }
    return ret;
}

static int mtb_verify(void)
{
    int ret = -1;

    if (g_mtb.version < 4) {
        // do nothing
        ret = 0;
    } else {
        ret = mtbv4_verify();
    }
    return ret;
}

int mtb_image_verify(const char *name)
{
    int ret = 0;

    if (name == NULL) {
        return -EINVAL;
    }

    if (strncmp(name, MTB_IMAGE_NAME_IMTB, MTB_IMAGE_NAME_SIZE) == 0) {
        ret = mtb_verify();
    } else {
#if (CONFIG_PARITION_NO_VERIFY == 0)
        if (g_mtb.version < 4) {
            ret = mtbv_image_verify(name);
        } else {
            ret = mtbv4_image_verify(name);
        }
#endif
    }
    return ret;
}

uint32_t mtb_get_bmtb_size(const uint8_t *data)
{
    mhead_tb *phead = (mhead_tb *)data;

    if (phead && phead->version == 1) {
        return phead->size;
    }
    return 0;
}

uint32_t mtb_get_size(void)
{
    uint32_t size = 0;
    mhead_tb *phead = (mhead_tb *)g_mtb.using_addr;

    size = phead->size;

    if (g_mtb.version == 1) {
        // offset to find imtb
        phead = (mhead_tb *)(g_mtb.using_addr + size);
        // add imtb size
        if (phead->magic == MTB_MAGIC)
            size += phead->size;
    }

    return size;
}

int mtb_get_partition_info(const char *name, mtb_partition_info_t *part_info)
{
    if (!(name && part_info)) {
        return -EINVAL;
    }
    if (g_mtb.version < 4) {
        uint32_t part_addr, part_size;
        if (mtbv_get_part_info((uint8_t *)name, &part_addr, &part_size)) {
            return -1;
        }
        memcpy(part_info->name, name, MTB_IMAGE_NAME_SIZE);
        part_info->start_addr = part_addr;
        part_info->end_addr = part_addr + part_size;
        return 0;
    } else {
        return mtbv4_get_partition_info(name, part_info);
    }
    return -EINVAL;
}

int mtb_get_img_info(const char *name, img_info_t *img_info)
{
    if (!(name && img_info)) {
        return -EINVAL;
    }
    if (mtb_get()->version == 4) {
        mtb_partition_info_t part_info;
        if (mtb_get_partition_info((const char *)name, &part_info)) {
            return -1;
        }
        strncpy((char *)(img_info->img_name), part_info.name, sizeof((img_info->img_name)));
        img_info->img_name[sizeof((img_info->img_name)) - 1] = '\0';
        img_info->img_addr =  part_info.start_addr;
        img_info->img_size =  part_info.img_size;
        img_info->img_type =  part_info.part_type.son_type;
        img_info->img_part_size = part_info.end_addr - part_info.start_addr;
    } else if (mtb_get()->version < 4) {
        return mtbv_get_img_info(name, img_info);
    } else {
        return -1;
    }

    return 0;
}

int get_section_buf(uint32_t addr, uint32_t img_len, scn_type_t *scn_type, uint32_t *scn_size)
{
    int ret;
    uint32_t size;
    uint8_t buf[sizeof(scn_head_t)];

    if (!(scn_type && scn_size && img_len > 0)) {
        return -EINVAL;
    }

    MTB_LOGD("addr:0x%x, img_len:%d", addr, img_len);
    ret = get_data_from_faddr(addr, buf, sizeof(scn_head_t));
    if (ret < 0) {
        return -1;
    }
    memcpy(&scn_type->father_type, buf, sizeof(scn_father_type_t));
    scn_type->son_type = buf[1];

    size = buf[4] + ((uint32_t)buf[5] << 8) + ((uint32_t)buf[6] << 16) + ((uint32_t)buf[7] << 24);
    MTB_LOGD("fatype=%d,sontype=%d,size=%d", *(uint8_t *)(&scn_type->father_type), scn_type->son_type, size);

    if (size > img_len) {
        MTB_LOGE("section size larger, size:%d, img_len:%d", size, img_len);
        return -1;
    }

    *scn_size = size;
    return 0;
}

int mtb_get_img_scn_addr(uint8_t *mtb_buf, const char *name, unsigned long *scn_addr)
{
    int ret;

    if (!(mtb_buf && name && scn_addr)) {
        return -EINVAL;
    }
    ret = get_section_addr((unsigned long)mtb_buf, &(g_scn_img_type[SCN_SUB_TYPE_IMG_NOR]), (uint8_t *)name,
                     (unsigned long *)scn_addr, SEACH_MODE_FIRST_TYPE);
    return ret;
}

int get_sys_partition(uint8_t *out, uint32_t *out_len)
{
    int ret = 0;

    MTB_LOGD("get_sys_partition----------");
    MTB_LOGD("mtb.prim_addr: 0x%lx", g_mtb.prim_addr);
    MTB_LOGD("mtb.backup_addr: 0x%lx", g_mtb.backup_addr);
    MTB_LOGD("mtb.using_addr: 0x%lx", g_mtb.using_addr);
    MTB_LOGD("mtb.one_size: 0x%x", g_mtb.one_size);
    MTB_LOGD("mtb.version: 0x%x", g_mtb.version);

    if (!(out && out_len)) {
        return -EINVAL;
    }

    if (g_mtb.version < 4) {
        scn_head_t *partition_addr;
        uint8_t *ret_addr = NULL;

        if (GET_PARTION_SCN_ADDR(g_mtb.using_addr, &ret_addr) < 0) {
            return -1;
        }
        partition_addr = (scn_head_t *)ret_addr;
        ret_addr += sizeof(scn_head_t);
        *out_len = partition_addr->size - sizeof(scn_head_t);
        memcpy(out, ret_addr, *out_len);
    } else {
#if !defined(CONFIG_MANTB_VERSION) || (CONFIG_MANTB_VERSION > 3)
        int i, count, len;
        sys_partition_info_t *p = (sys_partition_info_t *)out;

        count = mtbv4_partition_count();

        for (i = 0; i < count; i++) {
            mtb_partition_info_t part_info;
            mtbv4_get_partition_info_with_index(i, &part_info);
            //strcpy(p->image_name, part_info.name);
            len = strlen(part_info.name);
            len = len > (MTB_IMAGE_NAME_SIZE - 1) ? (MTB_IMAGE_NAME_SIZE - 1) : len;
            memcpy(p->image_name, part_info.name, len);
            p->image_name[len] = 0;

            p->part_addr = part_info.start_addr;
            p->part_size = part_info.end_addr - part_info.start_addr;
            p->image_size = part_info.img_size;
            p->load_addr = part_info.load_addr;
#if CONFIG_MULTI_FLASH_SUPPORT
            p->type = part_info.part_type;
#endif
            p ++;
        }
        *out_len = sizeof(sys_partition_info_t) * count;
#endif
    }

	return ret;
}

int get_app_version(uint8_t *out, uint32_t *out_len)
{
    int head_size;
    int version_max_size = MTB_OS_VERSION_LEN_V4;

    if (!(out && out_len)) {
        return -EINVAL;
    }

    if (g_mtb.version < 4) {
        head_size = sizeof(mhead_tb);
        version_max_size = MTB_OS_VERSION_LEN_V2;
        if (g_mtb.version == 1) {
            version_max_size = MTB_OS_VERSION_LEN;
        }
    } else {
#if !defined(CONFIG_MANTB_VERSION) || (CONFIG_MANTB_VERSION > 3)
        head_size = sizeof(imtb_head_v4_t);
        version_max_size = MTB_OS_VERSION_LEN_V4;
#endif
    }

    memcpy(out, (uint8_t *)(g_mtb.using_addr + head_size), version_max_size);
    out[version_max_size - 1] = 0;
    *out_len = version_max_size;
    return 0;
}

mtb_t *mtb_get(void)
{
    return &g_mtb;
}