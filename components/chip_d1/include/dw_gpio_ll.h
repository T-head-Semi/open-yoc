/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     dw_gpio_ll.h
 * @brief
 * @version
 * @date     2020-01-07
 ******************************************************************************/

#ifndef _DW_GPIO_H_
#define _DW_GPIO_H_

#include <soc.h>
#include <csi_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/* IS_SYNC, offset: 0x60 */
#define DW_GPIO_IS_SYNC_SYNC_LEVEL_Pos             (0U)
#define DW_GPIO_IS_SYNC_SYNC_LEVEL_Msk             (0x1U << DW_IIC_CON_MASTER_Pos)
#define DW_GPIO_IS_SYNC_SYNC_LEVEL                 DW_GPIO_IS_SYNC_SYNC_LEVEL_Msk


typedef struct {
    __IOM uint32_t SWPORTA_DR;                    /* Offset: 0x000 (W/R)  PortA data register */
    __IOM uint32_t SWPORTA_DDR;                   /* Offset: 0x004 (W/R)  PortA data direction register */
    __IOM uint32_t PORTA_CTL;                     /* Offset: 0x008 (W/R)  PortA source register */
    __IOM uint32_t SWPORTB_DR;                    /* Offset: 0x00c (W/R)  PortB data register */
    __IOM uint32_t SWPORTB_DDR;                   /* Offset: 0x010 (W/R)  PortB data direction register */
    __IOM uint32_t PORTB_CTL;                     /* Offset: 0x014 (W/R)  PortB source register */
    __IOM uint32_t SWPORTC_DR;                    /* Offset: 0x018 (W/R)  PortC data register */
    __IOM uint32_t SWPORTC_DDR;                   /* Offset: 0x01c (W/R)  PortC data direction register */
    __IOM uint32_t PORTC_CTL;                     /* Offset: 0x020 (W/R)  PortC source register */
    __IOM uint32_t SWPORTD_DR;                    /* Offset: 0x024 (W/R)  PortD data register */
    __IOM uint32_t SWPORTD_DDR;                   /* Offset: 0x028 (W/R)  PortD data direction register */
    __IOM uint32_t PORTD_CTL;                     /* Offset: 0x02c (W/R)  PortD source register */
    __IOM uint32_t INTEN;                         /* Offset: 0x030 (W/R)  Interrupt enable register */
    __IOM uint32_t INTMASK;                       /* Offset: 0x034 (W/R)  Interrupt mask register */
    __IOM uint32_t INTTYPE_LEVEL;                 /* Offset: 0x038 (W/R)  Interrupt level register */
    __IOM uint32_t INT_POLARITY;                  /* Offset: 0x03c (W/R)  Interrupt polarity register */
    __IM  uint32_t INTSTATUS;                     /* Offset: 0x040 (R)    Interrupt status of Port */
    __IM  uint32_t RAWINTSTATUS;                  /* Offset: 0x044 (W/R)  Raw interrupt status of Port */
    __IOM uint32_t DEBOUNCE;                      /* Offset: 0x048 (W/R)  Debounce enable register */
    __OM  uint32_t PORTA_EOI;                     /* Offset: 0x04c (W/R)  Port clear interrupt register */
    __IM  uint32_t EXT_PORTA;                     /* Offset: 0x050 (W/R)  PortA external port register */
    __IM  uint32_t EXT_PORTB;                     /* Offset: 0x054 (W/R)  PortB external port register */
    __IM  uint32_t EXT_PORTC;                     /* Offset: 0x058 (W/R)  PortC external port register */
    __IM  uint32_t EXT_PORTD;                     /* Offset: 0x05c (W/R)  PortD external port register */
    __IOM uint32_t LS_SYNC;                       /* Offset: 0x060 (W/R)  Level-sensitive synchronization enable register */
    __IM  uint32_t ID_CODE;                       /* Offset: 0x064 (R)    ID code register */
    __IOM  uint32_t RESERVED;                     /* Offset: 0x068 (W/R)  Reserve register */
    __IM  uint32_t COMP_VERSION;                  /* Offset: 0x06c (R)    Component Version register */
    __IM  uint32_t CONFIG_REG1;                   /* Offset: 0x070 (R)    Configuration Register 1 */
    __IM  uint32_t CONFIG_REG2;                   /* Offset: 0x074 (R)    Configuration Register 2 */
} dw_gpio_regs_t;

static inline void dw_gpio_write_output_port(dw_gpio_regs_t *port, uint32_t value)
{
    port->SWPORTA_DR = value;
}

static inline uint32_t dw_gpio_read_output_port(dw_gpio_regs_t *port)
{
    return port->SWPORTA_DR;
}

static inline void dw_gpio_set_port_direction(dw_gpio_regs_t *port, uint32_t value)
{
    port->SWPORTA_DDR = value;
}

static inline uint32_t dw_gpio_read_port_direction(dw_gpio_regs_t *port)
{
    return port->SWPORTA_DDR;
}

static inline void dw_gpio_set_port_source(dw_gpio_regs_t *port, uint32_t value)
{
    port->PORTA_CTL = value;
}

static inline uint32_t dw_gpio_read_port_source(dw_gpio_regs_t *port)
{
    return port->PORTA_CTL;
}

static inline void dw_gpio_set_port_irq(dw_gpio_regs_t *port, uint32_t value)
{
    port->INTEN = value;
}

static inline uint32_t dw_gpio_read_port_irq(dw_gpio_regs_t *port)
{
    return port->INTEN;
}

static inline void dw_gpio_set_port_msk(dw_gpio_regs_t *port, uint32_t value)
{
    port->INTMASK = value;
}

static inline uint32_t dw_gpio_read_port_msk(dw_gpio_regs_t *port)
{
    return port->INTMASK;
}

static inline void dw_gpio_set_port_irq_sensitive(dw_gpio_regs_t *port, uint32_t value)
{
    port->INTTYPE_LEVEL = value;
}

static inline uint32_t dw_gpio_read_port_irq_sensitive(dw_gpio_regs_t *port)
{
    return port->INTTYPE_LEVEL;
}

static inline void dw_gpio_set_port_irq_polarity(dw_gpio_regs_t *port, uint32_t value)
{
    port->INT_POLARITY = value;
}

static inline uint32_t dw_gpio_read_port_irq_polarity(dw_gpio_regs_t *port)
{
    return port->INT_POLARITY;
}

static inline uint32_t dw_gpio_read_port_int_status(dw_gpio_regs_t *port)
{
    return port->INTSTATUS;
}

static inline uint32_t dw_gpio_read_raw_port_int_status(dw_gpio_regs_t *port)
{
    return port->RAWINTSTATUS;
}

static inline void dw_gpio_clr_port_irq(dw_gpio_regs_t *port, uint32_t value)
{
    port->PORTA_EOI = value;
}

static inline uint32_t dw_gpio_read_input_port(dw_gpio_regs_t *port)
{
    return port->EXT_PORTA;
}

static inline void dw_gpio_set_sync_level(dw_gpio_regs_t *port, bool enable)
{
    port->LS_SYNC = enable;
}

static inline uint32_t dw_gpio_read_sync_level(dw_gpio_regs_t *port)
{
    return port->LS_SYNC;
}

static inline uint32_t dw_gpio_get_id_code(dw_gpio_regs_t *port)
{
    return port->ID_CODE;
}

static inline uint32_t dw_gpio_get_comp_version(dw_gpio_regs_t *port)
{
    return port->COMP_VERSION;
}

static inline uint32_t dw_gpio_get_parm1_status(dw_gpio_regs_t *port)
{
    return port->CONFIG_REG1;
}

static inline uint32_t dw_gpio_get_parm2_status(dw_gpio_regs_t *port)
{
    return port->CONFIG_REG2;
}

#ifdef __cplusplus
}
#endif

#endif

