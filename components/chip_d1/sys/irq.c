/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     irq.c
 * @brief    CSI Source File for IRQ Driver
 * @version  V1.0
 * @date     7. April 2020
 ******************************************************************************/

#include <stdint.h>
#include <soc.h>
#include <csi_core.h>
#include <drv/common.h>
#ifndef CONFIG_KERNEL_NONE
#include <csi_kernel.h>
#endif

extern void Default_Handler(void);
extern uint32_t soc_irq_get_irq_num(void);
extern void soc_irq_end(uint32_t irq_num);

#ifndef CONFIG_KERNEL_NONE
#define CSI_INTRPT_ENTER() csi_kernel_intrpt_enter()
#define CSI_INTRPT_EXIT()  csi_kernel_intrpt_exit()
#else
#define CSI_INTRPT_ENTER()
#define CSI_INTRPT_EXIT()
#endif

uint32_t  g_irq_nested_level;
csi_dev_t *g_irq_table[CONFIG_IRQ_NUM] = {0};

/**
  \brief       register irq handler.
  \param[in]   irq_num Number of IRQ.
  \return      None.
*/
// irqreturn_t d1_irq(int irq_num, void *dev)
// {
//     if (g_irq_table[irq_num] && g_irq_table[irq_num]->irq_handler) {
//         g_irq_table[irq_num]->irq_handler(g_irq_table[irq_num]);
//     }

//     return 0;
// }

void csi_irq_attach(uint32_t irq_num, void *irq_handler, csi_dev_t *dev)
{
    dev->irq_handler = irq_handler;
    g_irq_table[irq_num] = dev;

    // D1
    // request_irq(irq_num, d1_irq, 0, NULL, dev);
}

/**
  \brief       unregister irq handler.
  \param[in]   irq_num Number of IRQ.
  \param[in]   irq_handler IRQ Handler.
  \return      None.
*/
void csi_irq_detach(uint32_t irq_num)
{
    g_irq_table[irq_num] = NULL;
    // g_irq_table[irq_num]->irq_handler = (void *)Default_Handler;

    // D1
    // free_irq(irq_num, NULL);
}

/**
  \brief       gets whether in irq context
  \return      true or false.
*/
bool csi_irq_context(void)
{
    return ((g_irq_nested_level > 0U) ? true : false);
}

/**
  \brief       dispatching irq handlers
  \return      None.
*/
extern int generic_handle_irq(unsigned int irq);
__attribute__((section(".ram.code"))) __WEAK void do_irq(void)
{
    uint32_t irqn;
    uint32_t irqnc;

    g_irq_nested_level++;

    CSI_INTRPT_ENTER();

    irqnc = (__get_MCAUSE() & 0x3FF);
    irqn = irqnc;
    if (irqnc != CORET_IRQn) {                  // FIXME:
        irqn = soc_irq_get_irq_num();
    }

    if (g_irq_table[irqn] && g_irq_table[irqn]->irq_handler) {
        g_irq_table[irqn]->irq_handler(g_irq_table[irqn]);
    } else {
        if (generic_handle_irq(irqn) < 0) {
			// Default_Handler();
		}
    }

    CSI_INTRPT_EXIT();

    g_irq_nested_level --;

    if (irqnc != CORET_IRQn) {
        soc_irq_end(irqn);
    }
}
