/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drv/eflash.h>
#include <drv/spiflash.h>

#include "soc.h"
#include "aos/hal/flash.h"

#define EFLASH_SECTOR_SIZE 512

#ifndef CONFIG_FIX_ALIOS_THINGS_KV
#define CONFIG_FIX_ALIOS_THINGS_KV 0
#endif

/*===========implement for aiot ugly, fix me in the future===============*/
__attribute__((weak)) const hal_logic_partition_t hal_partitions[1];
static csi_eflash_t eflash_handle;
static csi_spiflash_t spiflash_handle;

#if CONFIG_FIX_ALIOS_THINGS_KV
int g_erase_buf[EFLASH_SECTOR_SIZE / 4];
#endif

static void _hal_flash_init(void)
{
    static int init;

    if (!init) {
        init = 1;
        csi_eflash_init(&eflash_handle, 0, NULL);
        csi_spiflash_qspi_init(&spiflash_handle, 0, NULL);
    }
}

int32_t hal_flash_info_get(hal_partition_t in_partition, hal_logic_partition_t *partition)
{
    _hal_flash_init();
    if (in_partition == HAL_ALL_FLASH_0) {
        csi_spiflash_info_t flash_info;
        csi_error_t ret = csi_spiflash_get_flash_info(&spiflash_handle, &flash_info);
        if (ret != CSI_OK) {
            return -1;
        }

        //FIXME: structure multiplex, attention!
        partition->partition_start_addr = flash_info.xip_addr;
        partition->partition_length     = flash_info.flash_size;
        partition->partition_options    = flash_info.flash_size / flash_info.sector_size;
    } else {
        hal_logic_partition_t *logic_partition;

        logic_partition = (hal_logic_partition_t *)&hal_partitions[ in_partition ];
        memcpy(partition, logic_partition, sizeof(hal_logic_partition_t));
    }

    return 0;
}

int32_t hal_flash_erase(hal_partition_t in_partition, uint32_t off_set, uint32_t size)
{
    int32_t ret;

    _hal_flash_init();
    if (in_partition == HAL_ALL_FLASH_0) {
        csi_spiflash_info_t flash_info;
        csi_error_t ret = csi_spiflash_get_flash_info(&spiflash_handle, &flash_info);
        if (ret != CSI_OK) {
            return -1;
        }

        ret = csi_spiflash_erase(&spiflash_handle, off_set, size);
        if (ret < 0) {
            return -1;
        }
    } else {
        uint32_t addr, flash_size;
        hal_logic_partition_t partition_info;
        hal_flash_t owner;
        csi_spiflash_info_t info;

        if (hal_flash_info_get(in_partition, &partition_info) != 0) {
            return -1;
        }

        if (csi_spiflash_get_flash_info(&spiflash_handle, &info) < 0U) {
            return -1;
        }

        addr = partition_info.partition_start_addr + off_set;
        addr -= info.xip_addr;
        owner = partition_info.partition_owner;
        flash_size = partition_info.partition_length;

        if(off_set + 1 > flash_size || size > flash_size || size + off_set > flash_size) {
            return -1;
        }

        if (owner == HAL_FLASH_EMBEDDED) {
#if CONFIG_FIX_ALIOS_THINGS_KV
            memset(g_erase_buf, 0xFF, EFLASH_SECTOR_SIZE);
#endif
            csi_eflash_erase(&eflash_handle, addr, size);
#if CONFIG_FIX_ALIOS_THINGS_KV
            csi_eflash_program(eflash_handle, addr + EFLASH_SECTOR_SIZE * i, g_erase_buf, EFLASH_SECTOR_SIZE);
#endif
        } else if (owner == HAL_FLASH_QSPI || owner == HAL_FLASH_SPI) {
            ret = csi_spiflash_erase(&spiflash_handle, addr, size);
            if (ret < 0) {
                return -1;
            }
        }
    }

    return 0;
}

int32_t hal_flash_write(hal_partition_t in_partition, uint32_t *off_set, const void *in_buf, uint32_t in_buf_len)
{
    _hal_flash_init();
    if (in_partition == HAL_ALL_FLASH_0) {
        csi_spiflash_program(&spiflash_handle, *off_set, in_buf, in_buf_len);
    } else {
        uint32_t addr, flash_size;
        hal_logic_partition_t partition_info;
        hal_flash_t owner;
        csi_spiflash_info_t info;

        if (off_set == NULL) {
            return -1;
        }

        if (in_buf == NULL) {
            return -1;
        }

        if (hal_flash_info_get(in_partition, &partition_info) != 0) {
            return -1;
        }

        if (csi_spiflash_get_flash_info(&spiflash_handle, &info) < 0U) {
            return -1;
        }
        addr = partition_info.partition_start_addr + *off_set;
        addr -= info.xip_addr;
        owner = partition_info.partition_owner;
        flash_size = partition_info.partition_length;

        if(*off_set + 1 > flash_size || in_buf_len > flash_size || in_buf_len + *off_set > flash_size) {
            return -1;
        }

        if (owner == HAL_FLASH_EMBEDDED) {
#if CONFIG_FIX_ALIOS_THINGS_KV
            memset(g_erase_buf, 0xFF, EFLASH_SECTOR_SIZE);

            if ((addr + in_buf_len) <= ((addr & 0xFFFFFE00) + EFLASH_SECTOR_SIZE)) {
                csi_eflash_read(eflash_handle, addr & 0xFFFFFE00, g_erase_buf, EFLASH_SECTOR_SIZE);
                memcpy((void *)((uint32_t)g_erase_buf + addr % EFLASH_SECTOR_SIZE), in_buf, in_buf_len);
                csi_eflash_erase_sector(eflash_handle, addr & 0xFFFFFE00);
                csi_eflash_program(eflash_handle, addr & 0xFFFFFE00, g_erase_buf, EFLASH_SECTOR_SIZE);
            } else {
                uint32_t remain_len = addr + in_buf_len - ((addr & 0xFFFFFE00) + EFLASH_SECTOR_SIZE);
                uint32_t len = in_buf_len - remain_len;

                csi_eflash_read(eflash_handle, addr & 0xFFFFFE00, g_erase_buf, EFLASH_SECTOR_SIZE);
                memcpy((void *)((uint32_t)g_erase_buf + addr % EFLASH_SECTOR_SIZE), in_buf, len);
                csi_eflash_erase_sector(eflash_handle, addr & 0xFFFFFE00);
                csi_eflash_program(eflash_handle, addr & 0xFFFFFE00, g_erase_buf, EFLASH_SECTOR_SIZE);

                memset(g_erase_buf, 0xFF, EFLASH_SECTOR_SIZE);
                csi_eflash_read(eflash_handle, (addr & 0xFFFFFE00) + EFLASH_SECTOR_SIZE, g_erase_buf, EFLASH_SECTOR_SIZE);
                memcpy((void *)g_erase_buf, (void *)((uint32_t)in_buf + len), remain_len);
                csi_eflash_erase_sector(eflash_handle, (addr & 0xFFFFFE00) + EFLASH_SECTOR_SIZE);
                csi_eflash_program(eflash_handle, (addr & 0xFFFFFE00) + EFLASH_SECTOR_SIZE, g_erase_buf, EFLASH_SECTOR_SIZE);
            }

#else
            csi_eflash_program(&eflash_handle, addr, in_buf, in_buf_len);
#endif
        } else if (owner == HAL_FLASH_QSPI || owner == HAL_FLASH_SPI) {
            csi_spiflash_program(&spiflash_handle, addr, in_buf, in_buf_len);
        }
    }

    return 0;
}

int32_t hal_flash_erase_write(hal_partition_t in_partition, uint32_t *off_set,
                              const void *in_buf, uint32_t in_buf_len)
{
    int32_t ret;

    _hal_flash_init();
    ret = hal_flash_erase(in_partition, *off_set, in_buf_len);
    ret = hal_flash_write(in_partition, off_set, in_buf, in_buf_len);

    return ret;
}

int32_t hal_flash_read(hal_partition_t in_partition, uint32_t *off_set, void *out_buf, uint32_t out_buf_len)
{
    _hal_flash_init();
    if (in_partition == HAL_ALL_FLASH_0) {
        int ret;

        ret = csi_spiflash_read(&spiflash_handle, *off_set, out_buf, out_buf_len);
        if (ret != out_buf_len) {
            return -1;
        }
    } else {
        uint32_t addr, flash_size;
        hal_logic_partition_t partition_info;
        hal_flash_t owner;
        csi_spiflash_info_t info;

        if (off_set == NULL) {
            return -1;
        }

        if (out_buf == NULL) {
            return -1;
        }

        if (hal_flash_info_get(in_partition, &partition_info) != 0) {
            return -1;
        }

        if (csi_spiflash_get_flash_info(&spiflash_handle, &info) < 0U) {
            return -1;
        }
        addr = partition_info.partition_start_addr + *off_set;
        addr -= info.xip_addr;
        owner = partition_info.partition_owner;
        flash_size = partition_info.partition_length;

        if(*off_set + 1 > flash_size || out_buf_len > flash_size || out_buf_len + *off_set > flash_size) {
            return -1;
        }

        if (owner == HAL_FLASH_EMBEDDED) {
            csi_eflash_read(&eflash_handle, addr, out_buf, out_buf_len);
        } else if (owner == HAL_FLASH_QSPI || owner == HAL_FLASH_SPI) {
            csi_spiflash_read(&spiflash_handle, addr, out_buf, out_buf_len);
        }
    }

    return 0;
}

