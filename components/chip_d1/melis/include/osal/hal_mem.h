#ifndef SUNXI_HAL_MEM_H
#define SUNXI_HAL_MEM_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#define __va_to_pa(vaddr) ((u32)vaddr)
#define __pa_to_va(vaddr) ((u32)vaddr)

void *hal_malloc(uint32_t size);
void hal_free(void *p);

void *hal_malloc_align(uint32_t size, int align);
void hal_free_align(void *p);

#ifdef __cplusplus
}
#endif
#endif
