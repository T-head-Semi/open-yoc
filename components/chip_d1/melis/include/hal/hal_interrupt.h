#ifndef SUNXI_HAL_INTERRUPT_H
#define SUNXI_HAL_INTERRUPT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
// #include <interrupt.h>

enum irqreturn
{
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};
typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (*irq_handler_t)(int, void *);

#define __must_check            __attribute__((__warn_unused_result__))
extern int __must_check
request_threaded_irq(unsigned int irq, irq_handler_t handler,
                     irq_handler_t thread_fn,
                     unsigned long flags, const char *name, void *dev);

static inline int __must_check request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
        const char *name, void *dev)
{
    return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}

// int request_irq(unsigned int irq, irq_handler_t handler,
// 		unsigned long flags, const char *name, void *dev);

void *free_irq(unsigned int irq, void *data);

void enable_irq(unsigned int irq);

void disable_irq(unsigned int irq);


void hal_interrupt_enable(void);
void hal_interrupt_disable(void);
uint32_t hal_interrupt_save(void);
void hal_interrupt_restore(uint32_t flag);

uint32_t hal_interrupt_get_nest(void);

#ifdef __cplusplus
}
#endif

#endif
