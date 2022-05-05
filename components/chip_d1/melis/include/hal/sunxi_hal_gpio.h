/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY��S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS��SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __GPIO_H__
#define __GPIO_H__

#include <log.h>
#include <stdlib.h>
// #include <interrupt.h>
#include "sunxi_hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_DRIVERS_GPIO_DEBUG
#ifndef CONFIG_DRIVERS_GPIO_DEBUG
#define GPIO_INFO(fmt, arg...) __log(fmt, ##arg)
#else
#define GPIO_INFO(fmt, arg...) do {}while(0)
#endif

#define GPIO_ERR(fmt, arg...) __err(fmt, ##arg)

#define BANK_MEM_SIZE       0x24
#define MUX_REGS_OFFSET     0x0
#define DATA_REGS_OFFSET    0x10
#define DLEVEL_REGS_OFFSET  0x14
#define PULL_REGS_OFFSET    0x1c

#define PINS_PER_BANK       32
#define MUX_PINS_PER_REG    8
#define MUX_PINS_BITS       4
#define MUX_PINS_MASK       0x0f
#define DATA_PINS_PER_REG   32
#define DATA_PINS_BITS      1
#define DATA_PINS_MASK      0x01
#define DLEVEL_PINS_PER_REG 16
#define DLEVEL_PINS_BITS    2
#define DLEVEL_PINS_MASK    0x03
#define PULL_PINS_PER_REG   16
#define PULL_PINS_BITS      2
#define PULL_PINS_MASK      0x03

#define IRQ_PER_BANK        32

#define IRQ_CFG_REG     0x200
#define IRQ_CFG_IRQ_PER_REG     8
#define IRQ_CFG_IRQ_BITS        4
#define IRQ_CFG_IRQ_MASK        ((1 << IRQ_CFG_IRQ_BITS) - 1)
#define IRQ_CTRL_REG        0x210
#define IRQ_CTRL_IRQ_PER_REG        32
#define IRQ_CTRL_IRQ_BITS       1
#define IRQ_CTRL_IRQ_MASK       ((1 << IRQ_CTRL_IRQ_BITS) - 1)
#define IRQ_STATUS_REG      0x214
#define IRQ_STATUS_IRQ_PER_REG      32
#define IRQ_STATUS_IRQ_BITS     1
#define IRQ_STATUS_IRQ_MASK     ((1 << IRQ_STATUS_IRQ_BITS) - 1)
#define IRQ_DEBOUNCE_REG        0x218
#define POWER_MODE_SEL 0x0340
#define POWER_MODE_VAL 0x0348
#define POWER_VOL_SEL 0x0350

#define IRQ_MEM_SIZE        0x20
#define GIC_IRQ_NUM     140
#define GPIO_IRQ_START      (GIC_IRQ_NUM + 1)

#define IRQ_EDGE_RISING     0x00
#define IRQ_EDGE_FALLING    0x01
#define IRQ_LEVEL_HIGH      0x02
#define IRQ_LEVEL_LOW       0x03
#define IRQ_EDGE_BOTH       0x04

#define PA_BASE 0
#define PB_BASE 32
#define PC_BASE 64
#define PD_BASE 96
#define PE_BASE 128
#define PF_BASE 160
#define PG_BASE 192
#define PH_BASE 224
#define PI_BASE 256
#define PJ_BASE 288
#define PK_BASE 320
#define PL_BASE 352
#define PM_BASE 384
#define PN_BASE 416
#define PO_BASE 448
#define GPIO_MAX_BANK PL_BASE
#define SUNXI_GPIO_PBASE 0x0300B000
#define SUXNI_GPIO_R_PBASE 0x07022000


/* sunxi gpio name space */
#define GPIOA(n)    (PA_BASE + (n))
#define GPIOB(n)    (PB_BASE + (n))
#define GPIOC(n)    (PC_BASE + (n))
#define GPIOD(n)    (PD_BASE + (n))
#define GPIOE(n)    (PE_BASE + (n))
#define GPIOF(n)    (PF_BASE + (n))
#define GPIOG(n)    (PG_BASE + (n))
#define GPIOH(n)    (PH_BASE + (n))
#define GPIOI(n)    (PI_BASE + (n))
#define GPIOJ(n)    (PJ_BASE + (n))
#define GPIOK(n)    (PK_BASE + (n))
#define GPIOL(n)    (PL_BASE + (n))
#define GPIOM(n)    (PM_BASE + (n))
#define GPION(n)    (PN_BASE + (n))
#define GPIOO(n)    (PO_BASE + (n))

/* sunxi gpio irq*/
#define SUNXI_GIC_START 32
#define SUNXI_IRQ_GPIOC (SUNXI_GIC_START + 67)
#define SUNXI_IRQ_GPIOD (SUNXI_GIC_START + 68)
#define SUNXI_IRQ_GPIOE (SUNXI_GIC_START + 69)
#define SUNXI_IRQ_GPIOF (SUNXI_GIC_START + 70)
#define SUNXI_IRQ_GPIOG (SUNXI_GIC_START + 71)
#define SUNXI_IRQ_GPIOH (SUNXI_GIC_START + 72)
#define SUNXI_IRQ_GPIOI (SUNXI_GIC_START + 73)
#define SUNXI_IRQ_R_GPIOL (SUNXI_GIC_START + 106)


#define SUNXI_PIO_BANK_BASE(pin, irq_bank) \
    ((pin-PA_BASE)/PINS_PER_BANK - irq_bank)

#define SUNXI_R_PIO_BANK_BASE(pin, irq_bank) \
    ((pin-PL_BASE)/PINS_PER_BANK - irq_bank)

/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/*
 * gpio configuration (pull up/down and drive strength) type and its value are
 * packed together into a 32-bits. The lower 8-bits represent the configuration
 * type and the upper 24-bits hold the value of the configuration type.
 */
#define GPIO_CFG_PACK(type, value)  (((value) << 8) | ((unsigned long) type & 0xFFUL))
#define GPIO_CFG_UNPACK_TYPE(cfg)   ((cfg) & 0xFFUL)
#define GPIO_CFG_UNPACK_VALUE(cfg)  (((cfg) & 0xFFFFFF00UL) >> 8)

typedef enum
{
    GPIO_PC0 = GPIOC(0),
    GPIO_PC1 = GPIOC(1),
    GPIO_PC2 = GPIOC(2),
    GPIO_PC3 = GPIOC(3),
    GPIO_PC4 = GPIOC(4),
    GPIO_PC5 = GPIOC(5),
    GPIO_PC6 = GPIOC(6),
    GPIO_PC7 = GPIOC(7),
    GPIO_PC8 = GPIOC(8),
    GPIO_PC9 = GPIOC(9),
    GPIO_PC10 = GPIOC(10),
    GPIO_PC11 = GPIOC(11),

    GPIO_PD0 = GPIOD(0),
    GPIO_PD1 = GPIOD(1),
    GPIO_PD2 = GPIOD(2),
    GPIO_PD3 = GPIOD(3),
    GPIO_PD4 = GPIOD(4),
    GPIO_PD5 = GPIOD(5),
    GPIO_PD6 = GPIOD(6),
    GPIO_PD7 = GPIOD(7),
    GPIO_PD8 = GPIOD(8),
    GPIO_PD9 = GPIOD(9),
    GPIO_PD10 = GPIOD(10),
    GPIO_PD11 = GPIOD(11),
    GPIO_PD12 = GPIOD(12),
    GPIO_PD13 = GPIOD(13),
    GPIO_PD14 = GPIOD(14),
    GPIO_PD15 = GPIOD(15),
    GPIO_PD16 = GPIOD(16),
    GPIO_PD17 = GPIOD(17),
    GPIO_PD18 = GPIOD(18),
    GPIO_PD19 = GPIOD(19),
    GPIO_PD20 = GPIOD(20),
    GPIO_PD21 = GPIOD(21),
    GPIO_PD22 = GPIOD(22),

    GPIO_PE0 = GPIOE(0),
    GPIO_PE1 = GPIOE(1),
    GPIO_PE2 = GPIOE(2),
    GPIO_PE3 = GPIOE(3),
    GPIO_PE4 = GPIOE(4),
    GPIO_PE5 = GPIOE(5),
    GPIO_PE6 = GPIOE(6),
    GPIO_PE7 = GPIOE(7),
    GPIO_PE8 = GPIOE(8),
    GPIO_PE9 = GPIOE(9),
    GPIO_PE10 = GPIOE(10),
    GPIO_PE11 = GPIOE(11),
    GPIO_PE12 = GPIOE(12),
    GPIO_PE13 = GPIOE(13),
    GPIO_PE14 = GPIOE(14),
    GPIO_PE15 = GPIOE(15),
    GPIO_PE16 = GPIOE(16),
    GPIO_PE17 = GPIOE(17),
    GPIO_PE18 = GPIOE(18),
    GPIO_PE19 = GPIOE(19),
    GPIO_PE20 = GPIOE(20),
    GPIO_PE21 = GPIOE(21),

    GPIO_PF0 = GPIOF(0),
    GPIO_PF1 = GPIOF(1),
    GPIO_PF2 = GPIOF(2),
    GPIO_PF3 = GPIOF(3),
    GPIO_PF4 = GPIOF(4),
    GPIO_PF5 = GPIOF(5),
    GPIO_PF6 = GPIOF(6),

    GPIO_PG0 = GPIOG(0),
    GPIO_PG1 = GPIOG(1),
    GPIO_PG2 = GPIOG(2),
    GPIO_PG3 = GPIOG(3),
    GPIO_PG4 = GPIOG(4),
    GPIO_PG5 = GPIOG(5),
    GPIO_PG6 = GPIOG(6),
    GPIO_PG7 = GPIOG(7),
	GPIO_PG8 = GPIOG(8),
    GPIO_PG9 = GPIOG(9),
    GPIO_PG10 = GPIOG(10),
    GPIO_PG11 = GPIOG(11),
    GPIO_PG12 = GPIOG(12),

    GPIO_PH0 = GPIOH(0),
    GPIO_PH1 = GPIOH(1),
    GPIO_PH2 = GPIOH(2),
    GPIO_PH3 = GPIOH(3),
    GPIO_PH4 = GPIOH(4),
    GPIO_PH5 = GPIOH(5),
    GPIO_PH6 = GPIOH(6),
    GPIO_PH7 = GPIOH(7),
    GPIO_PH8 = GPIOH(8),
    GPIO_PH9 = GPIOH(9),
    GPIO_PH10 = GPIOH(10),
    GPIO_PH11 = GPIOH(11),
    GPIO_PH12 = GPIOH(12),
    GPIO_PH13 = GPIOH(13),
    GPIO_PH14 = GPIOH(14),
    GPIO_PH15 = GPIOH(15),

    GPIO_PI0 = GPIOI(0),
    GPIO_PI1 = GPIOI(1),
    GPIO_PI2 = GPIOI(2),
    GPIO_PI3 = GPIOI(3),
    GPIO_PI4 = GPIOI(4),
    GPIO_PI5 = GPIOI(5),

    GPIO_PL0 = GPIOL(0),
    GPIO_PL1 = GPIOL(1),
    GPIO_PL2 = GPIOL(2),
    GPIO_PL3 = GPIOL(3),
    GPIO_PL4 = GPIOL(4),
    GPIO_PL5 = GPIOL(5),
} gpio_pin_t;

/**This enum defines the GPIO MUX function*/
typedef enum
{
    GPIO_MUXSEL_IN = 0,
    GPIO_MUXSEL_OUT = 1,
    GPIO_MUXSEL_FUNCTION2 = 2,
    GPIO_MUXSEL_FUNCTION3 = 3,
    GPIO_MUXSEL_FUNCTION4 = 4,
    GPIO_MUXSEL_FUNCTION5 = 5,
    GPIO_MUXSEL_FUNCTION6 = 6,
    GPIO_MUXSEL_DISABLED = 7,
} gpio_muxsel_t;

typedef enum
{
    GPIO_DRIVING_LEVEL0    = 0,        /**< Defines GPIO driving current as level0.  */
    GPIO_DRIVING_LEVEL1    = 1,        /**< Defines GPIO driving current as level1.  */
    GPIO_DRIVING_LEVEL2    = 2,        /**< Defines GPIO driving current as level2. */
    GPIO_DRIVING_LEVEL3    = 3         /**< Defines GPIO driving current as level3. */
} gpio_driving_level_t;

typedef enum
{
    GPIO_PULL_DOWN_DISABLED    = 0,        /**< Defines GPIO pull up and pull down disable.  */
    GPIO_PULL_UP          = 1,        /**< Defines GPIO is pull up state.  */
    GPIO_PULL_DOWN            = 2,        /**< Defines GPIO is pull down state. */
} gpio_pull_status_t;

/** This enum defines the GPIO direction. */
typedef enum
{
    GPIO_DIRECTION_INPUT  = 0,              /**<  GPIO input direction. */
    GPIO_DIRECTION_OUTPUT = 1               /**<  GPIO output direction. */
} gpio_direction_t;

/** This enum defines the data type of GPIO. */
typedef enum
{
    GPIO_DATA_LOW  = 0,                     /**<  GPIO data low. */
    GPIO_DATA_HIGH = 1                      /**<  GPIO data high. */
} gpio_data_t;

typedef enum
{
    POWER_MODE_330 = 0,
    POWER_MODE_180 = 1
} gpio_power_mode_t;

typedef enum
{
    GPIO_TYPE_FUNC,
    GPIO_TYPE_DAT,
    GPIO_TYPE_PUD,
    GPIO_TYPE_DRV,
    GPIO_TYPE_VOL,
    GPIO_CONFIG_END = 0x7F,
    GPIO_CONFIG_MAX = 0xFF,
} pin_config_param_t;

typedef enum
{
    IRQ_TYPE_NONE       = 0x00000000,
    IRQ_TYPE_EDGE_RISING    = 0x00000001,
    IRQ_TYPE_EDGE_FALLING   = 0x00000002,
    IRQ_TYPE_EDGE_BOTH  = (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
    IRQ_TYPE_LEVEL_HIGH = 0x00000004,
    IRQ_TYPE_LEVEL_LOW  = 0x00000008,
} gpio_interrupt_mode_t;

struct gpio_irq_desc
{
    uint32_t virq;
    uint32_t pin;
    unsigned long flags;
    irq_handler_t handle_irq;
    void *data;
};

struct gpio_desc
{
    const uint32_t membase;
    const uint32_t irq_arry_size;
    const uint32_t *irq;
    const uint32_t pin_base;
    const uint32_t banks;
    const uint32_t *bank_base;
    const uint32_t irq_banks;
    const uint32_t *irq_bank_base;
    const uint32_t virq_offset;
    uint32_t irq_desc_size;
    struct gpio_irq_desc *irq_desc;
};

bool hal_check_gpio_valid(gpio_pin_t pin);
int hal_gpio_get_data(gpio_pin_t pin, gpio_data_t *data);
int hal_gpio_set_data(gpio_pin_t pin, gpio_data_t data);
int hal_gpio_set_direction(gpio_pin_t pin, gpio_direction_t direction);
int hal_gpio_get_direction(gpio_pin_t pin, gpio_direction_t *direction);
int hal_gpio_set_pull(gpio_pin_t pin, gpio_pull_status_t pull);
int hal_gpio_get_pull(gpio_pin_t pin, gpio_pull_status_t *pull);
int hal_gpio_set_driving_level(gpio_pin_t pin, gpio_driving_level_t level);
int hal_gpio_get_driving_level(gpio_pin_t pin, gpio_driving_level_t *level);
int hal_gpio_pinmux_set_function(gpio_pin_t pin, gpio_muxsel_t function_index);
int hal_gpio_sel_vol_mode(gpio_pin_t pins, gpio_power_mode_t pm_sel);
int hal_gpio_set_debounce(gpio_pin_t pin, unsigned value);
int hal_gpio_to_irq(gpio_pin_t pin, uint32_t *irq);
int hal_gpio_irq_request(uint32_t irq, irq_handler_t hdle, unsigned long flags, void *data);
int hal_gpio_irq_free(uint32_t irq);
int hal_gpio_irq_enable(uint32_t irq);
int hal_gpio_irq_disable(uint32_t irq);
int hal_gpio_init(void);

#ifdef __cplusplus
}
#endif
#endif
