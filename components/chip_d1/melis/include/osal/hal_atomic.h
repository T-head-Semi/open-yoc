#ifndef SUNXI_HAL_ATOMIC_H
#define SUNXI_HAL_ATOMIC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

typedef unsigned long hal_spinlock_t;

void hal_spin_lock(hal_spinlock_t *lock);
void hal_spin_unlock(hal_spinlock_t *lock);

uint32_t hal_spin_lock_irqsave(hal_spinlock_t *lock);
void hal_spin_unlock_irqrestore(hal_spinlock_t *lock, uint32_t cpsr);

#ifdef __cplusplus
}
#endif
#endif
