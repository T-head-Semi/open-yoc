/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include "key_mgr_port.h"
#include <sec_crypto_sha.h>
#include <yoc/partition.h>
#include <yoc/partition_flash.h>

int get_data_from_addr(unsigned long addr, uint8_t *data, size_t data_len)
{
#ifdef CONFIG_NON_ADDRESS_FLASH
    void *handle;
    int flashid = 0;

#if CONFIG_MULTI_FLASH_SUPPORT
    flashid = get_flashid_by_abs_addr(addr);
#endif
    handle = partition_flash_open(flashid);
    partition_flash_read(handle, addr, data, data_len);
    partition_flash_close(handle);
#else
    memcpy(data, (uint8_t *)addr, data_len);
#endif /* CONFIG_NON_ADDRESS_FLASH */

    return 0;
}

