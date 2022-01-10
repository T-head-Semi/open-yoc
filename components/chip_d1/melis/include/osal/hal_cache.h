#ifndef SUNXI_HAL_CACHE_H
#define SUNXI_HAL_CACHE_H

#include <drv/porting.h>

#ifdef __cplusplus
extern "C"
{
#endif


static inline void hal_dcache_clean(unsigned long vaddr_start, unsigned long size)
{
    soc_dcache_clean_range(vaddr_start, size);
}

static inline void hal_dcache_invalidate(unsigned long vaddr_start, unsigned long size)
{
    soc_dcache_invalid_range(vaddr_start, size);
}

static inline void hal_dcache_clean_invalidate(unsigned long vaddr_start, unsigned long size)
{
    soc_dcache_clean_invalid_range(vaddr_start, size);
}

static inline void hal_icache_invalidate_all(void)
{
    soc_icache_invalid();
}

static inline void hal_dcache_invalidate_all(void)
{
    soc_dcache_invalid();
}

static inline void hal_dcache_clean_all(void)
{
    soc_dcache_clean();
}


#ifdef __cplusplus
}
#endif
#endif
