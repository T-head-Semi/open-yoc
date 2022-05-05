/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     weak.c
 * @brief    source file for the weak
 * @version  V1.0
 * @date     04. April 2019
 ******************************************************************************/

#include <csi_config.h>
#include <soc.h>
#include <csi_core.h>

__WEAK void soc_dcache_clean_invalid_range(unsigned long addr, uint32_t size)
{
    csi_dcache_clean_invalid_range((uint64_t *)addr, size);
}

__WEAK void soc_dcache_clean_invalid_all(void)
{
}

__WEAK void soc_dcache_invalid(void)
{
    csi_dcache_invalid();
}

__WEAK void soc_dcache_invalid_range(unsigned long addr, uint32_t size)
{
    csi_dcache_invalid_range((uint64_t *)addr, size);
}

__WEAK void soc_dcache_clean(void)
{
    csi_dcache_clean();
}

__WEAK void soc_dcache_clean_range(unsigned long addr, uint32_t size)
{
    csi_dcache_clean_range((uint64_t *)addr, size);
}

__WEAK void soc_icache_invalid(void)
{
    csi_icache_invalid();
}

__WEAK unsigned long soc_dma_address_remap(unsigned long addr)
{
    return addr;
}
