/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     soc.h
 * @brief    CSI Core Peripheral Access Layer Header File for
 *           CSKYSOC Device Series
 * @version  V1.0
 * @date     7. April 2020
 ******************************************************************************/

#ifndef _SOC_H_
#define _SOC_H_

#include <stdint.h>
#include <csi_core.h>
#include <sys_clk.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 0
/* ================================================================================ */
/* ================       Device Specific Peripheral Section       ================ */
/* ================================================================================ */

#define CONFIG_TIMER_NUM    4
#define CONFIG_USART_NUM    1
#define CONFIG_GPIO_NUM     8
#define CONFIG_GPIO_PIN_NUM 8

/* ================================================================================ */
/* ================              Peripheral memory map             ================ */
/* ================================================================================ */
/* --------------------------  CPU FPGA memory map  ------------------------------- */
#define CSKY_SRAM_BASE              (0x20000000UL)

#define CSKY_UART_BASE              (0x02500000UL)
#define CSKY_PMU_BASE               (0x10016000UL)
#define CSKY_TIMER0_BASE            (0x10011000UL)
#define CSKY_TIMER1_BASE            (0x10011014UL)
#define CSKY_TIMER2_BASE            (0x10011028UL)
#define CSKY_TIMER3_BASE            (0x1001103cUL)
#define CSKY_TIMER_CONTROL_BASE     (0x100110a0UL)
#define CSKY_CLK_GEN_BASE           (0x10017000UL)
#define CSKY_STIMER0_BASE           (0x10018000UL)
#define CSKY_STIMER1_BASE           (0x10018014UL)
#define CSKY_STIMER2_BASE           (0x10018028UL)
#define CSKY_STIMER3_BASE           (0x1001803cUL)
#define CSKY_STIMER_CONTROL_BASE    (0x100110a0UL)

#define CSKY_GPIOA_BASE             (0x10019000UL)
#define CSKY_GPIOA_CONTROL_BASE     (0x10019030UL)
#define CSKY_SMPU_BASE              (0x1001a000UL)

/* ================================================================================ */
/* ================             Peripheral declaration             ================ */
/* ================================================================================ */
#define CSKY_UART                  ((   CSKY_UART_TypeDef *)    CSKY_UART_BASE)

typedef enum IRQn {
    /* ----------------------  SmartL Specific Interrupt Numbers  --------------------- */
    UART_IRQn                       =   18+0,     /* uart Interrupt */
    TIM0_IRQn                       =   32+2,     /* timer0 Interrupt */
    TIM1_IRQn                       =   32+3,     /* timer1 Interrupt */
    TIM2_IRQn                       =   32+4,     /* timer1 Interrupt */
    TIM3_IRQn                       =   32+5,     /* timer1 Interrupt */
    GPIO0_IRQn                      =   32+6,     /* gpio0 Interrupt */
    GPIO1_IRQn                      =   32+7,     /* gpio1 Interrupt */
    GPIO2_IRQn                      =   32+8,     /* gpio2 Interrupt */
    GPIO3_IRQn                      =   32+9,     /* gpio3 Interrupt */
    GPIO4_IRQn                      =   32+10,     /* gpio4 Interrupt */
    GPIO5_IRQn                      =   32+11,     /* gpio5 Interrupt */
    GPIO6_IRQn                      =   32+12,     /* gpio6 Interrupt */
    GPIO7_IRQn                      =   32+13,     /* gpio7 Interrupt */
    STIM0_IRQn                      =   32+14,     /* stimer0 Interrupt */
    STIM1_IRQn                      =   32+15,     /* stimer0 Interrupt */
    STIM2_IRQn                      =   32+16,     /* stimer0 Interrupt */
    STIM3_IRQn                      =   32+17,     /* stimer0 Interrupt */
    PAD_IRQn                        =   32+18,     /* pad Interrupt */
}
IRQn_Type;
#endif

#ifndef EHS_VALUE
#define EHS_VALUE               20000000U
#endif

#ifndef ELS_VALUE
#define ELS_VALUE               32768U
#endif

#ifndef IHS_VALUE
#define IHS_VALUE               24000000U
#endif

#ifndef ILS_VALUE
#define ILS_VALUE               32768U
#endif

////////////////////////////////////////////////////////
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

#define GPIO_MAX_BANK PG_BASE
#define BANK_BOUNDARY PL_BASE
#define SUNXI_GPIO_PBASE 0x02000000
#define SUNXI_GPIO_RES_SIZE 0x07FF

/* sunxi gpio irq*/
#if defined(CONFIG_CORE_DSP0) /* DSP */
#include <interrupt.h>
#define SUNXI_IRQ_GPIOB (RINTC_IRQ_MASK | 40)
#define SUNXI_IRQ_GPIOC (RINTC_IRQ_MASK | 42)
#define SUNXI_IRQ_GPIOD (RINTC_IRQ_MASK | 44)
#define SUNXI_IRQ_GPIOE (RINTC_IRQ_MASK | 46)
#define SUNXI_IRQ_GPIOF (RINTC_IRQ_MASK | 48)
#define SUNXI_IRQ_GPIOG (RINTC_IRQ_MASK | 50)
#elif defined(CONFIG_ARCH_SUN8IW20) /* ARM */
#define SUNXI_GIC_START 32
#define SUNXI_IRQ_GPIOB (SUNXI_GIC_START + 69)
#define SUNXI_IRQ_GPIOC (SUNXI_GIC_START + 71)
#define SUNXI_IRQ_GPIOD (SUNXI_GIC_START + 73)
#define SUNXI_IRQ_GPIOE (SUNXI_GIC_START + 75)
#define SUNXI_IRQ_GPIOF (SUNXI_GIC_START + 77)
#define SUNXI_IRQ_GPIOG (SUNXI_GIC_START + 79)
#elif defined(CONFIG_SOC_SUN20IW1) /* RISC-V */
#define SUNXI_IRQ_GPIOB (85)
#define SUNXI_IRQ_GPIOC (87)
#define SUNXI_IRQ_GPIOD (89)
#define SUNXI_IRQ_GPIOE (91)
#define SUNXI_IRQ_GPIOF (93)
#define SUNXI_IRQ_GPIOG (95)
#endif /* CONFIG_CORE_DSP0 */

typedef enum
{
    GPIO_PB0 = GPIOB(0),
    GPIO_PB1 = GPIOB(1),
    GPIO_PB2 = GPIOB(2),
    GPIO_PB3 = GPIOB(3),
    GPIO_PB4 = GPIOB(4),
    GPIO_PB5 = GPIOB(5),
    GPIO_PB6 = GPIOB(6),
    GPIO_PB7 = GPIOB(7),

    GPIO_PC0 = GPIOC(0),
    GPIO_PC1 = GPIOC(1),
    GPIO_PC2 = GPIOC(2),
    GPIO_PC3 = GPIOC(3),
    GPIO_PC4 = GPIOC(4),
    GPIO_PC5 = GPIOC(5),
    GPIO_PC6 = GPIOC(6),
    GPIO_PC7 = GPIOC(7),

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
    GPIO_PG13 = GPIOG(13),
    GPIO_PG14 = GPIOG(14),
    GPIO_PG15 = GPIOG(15),

    /* To aviod compile warnings. */
    GPIO_MAX = GPIOO(31),

} gpio_pin_t;
////////////////////////////////////////////////////////
typedef gpio_pin_t pin_name_t;

typedef enum {
    Supervisor_Software_IRQn        =  1U,
    Machine_Software_IRQn           =  3U,
    Supervisor_Timer_IRQn           =  5U,
    CORET_IRQn                      =  7U,
    Supervisor_External_IRQn        =  9U,
    Machine_External_IRQn           =  11U,
    // FIXME:
    DW_WDT_IRQn                     =  17U,
    DW_UART_IRQn                    =  18U,
    DW_DMA_IRQn                     =  19U,
    DW_TIMER0_IRQn                  =  20U,
    DW_TIMER1_IRQn                  =  21U,
    DW_TIMER2_IRQn                  =  22U,
    DW_TIMER3_IRQn                  =  23U,
    WJ_VAD_IRQn                     =  24U,
    WJ_VAD_TRIG_IRQn                =  25U,
    WJ_I2S0_IRQn                    =  26U,
    WJ_I2S1_IRQn                    =  27U,
    WJ_I2S2_IRQn                    =  28U,
    WJ_I2S_IN_IRQn                  =  29U,
    WJ_TDM_IRQn                     =  30U,
    WJ_SPDIF0_IRQn                  =  31U,
    WJ_SPDIF1_IRQn                  =  32U,
    DW_GPIO_IRQn                    =  33U,
    DW_IIC0_IRQn                    =  34U,
    DW_IIC1_IRQn                    =  35U,
    WJ_MAILBOX_IRQn                 =  36U,
    WJ_BUS_MONITOR_IRQn             =  37U,
    WJ_CPR_WK_IRQn                  =  38U,
    WJ_NPU_IRQn                     =  39U,
} irqn_type_t;

typedef enum {
    WJ_IOCTL_Wakeupn               =  29U,     /* IOCTOL wakeup */
} wakeupn_type_t;

typedef enum {
    WJ_USB_CLK_MANAGERN            = 28U,
} clk_manager_type_t;

typedef enum {
    WJ_VAD_RX0_L,
    WJ_VAD_RX0_R,
    WJ_VAD_RX1_L,
    WJ_VAD_RX1_R,
    WJ_VAD_RX2_L,
    WJ_VAD_RX2_R,
    WJ_VAD_RX3_L,
    WJ_VAD_RX3_R,
    WJ_I2S0_RX,
    WJ_I2S0_TX,
    WJ_I2S1_RX,
    WJ_I2S1_TX,
    WJ_I2S2_RX,
    WJ_I2S2_TX,
    WJ_I2S_IN_RX0,
    WJ_I2S_IN_RX1,
    WJ_I2S_IN_RX2,
    WJ_I2S_IN_RX3,
    UART_RX,
    UART_TX,
    IIC0_RX,
    IIC0_TX,
    IIC1_RX,
    IIC1_TX,
    SPDIF0_RX,
    SPDIF0_TX,
    SPDIF1_RX,
    SPDIF1_TX,
    TDM_RX0,
    TDM_RX1,
    TDM_RX2,
    TDM_RX3,
    TDM_RX4,
    TDM_RX5,
    TDM_RX6,
    TDM_RX7,
} dman_type_t;

// FIXME:
typedef enum {
    PB0 = GPIO_PB0,
    PB1 = GPIO_PB1,
    PB2 = GPIO_PB2,
    PB3 = GPIO_PB3,
    PB4 = GPIO_PB4,
    PB5 = GPIO_PB5,
    PB6 = GPIO_PB6,
    PB7 = GPIO_PB7,

    PC0 = GPIO_PC0,
    PC1 = GPIO_PC1,
    PC2 = GPIO_PC2,
    PC3 = GPIO_PC3,
    PC4 = GPIO_PC4,
    PC5 = GPIO_PC5,
    PC6 = GPIO_PC6,
    PC7 = GPIO_PC7,

    PD0 = GPIO_PD0,
    PD1 = GPIO_PD1,
    PD2 = GPIO_PD2,
    PD3 = GPIO_PD3,
    PD4 = GPIO_PD4,
    PD5 = GPIO_PD5,
    PD6 = GPIO_PD6,
    PD7 = GPIO_PD7,
    PD8 = GPIO_PD8,
    PD9 = GPIO_PD9,
    PD10 = GPIO_PD10,
    PD11 = GPIO_PD11,
    PD12 = GPIO_PD12,
    PD13 = GPIO_PD13,
    PD14 = GPIO_PD14,
    PD15 = GPIO_PD15,
    PD16 = GPIO_PD16,
    PD17 = GPIO_PD17,
    PD18 = GPIO_PD18,
    PD19 = GPIO_PD19,
    PD20 = GPIO_PD20,
    PD21 = GPIO_PD21,
    PD22 = GPIO_PD22,

    PE0 = GPIO_PE0,
    PE1 = GPIO_PE1,
    PE2 = GPIO_PE2,
    PE3 = GPIO_PE3,
    PE4 = GPIO_PE4,
    PE5 = GPIO_PE5,
    PE6 = GPIO_PE6,
    PE7 = GPIO_PE7,
    PE8 = GPIO_PE8,
    PE9 = GPIO_PE9,
    PE10 = GPIO_PE10,
    PE11 = GPIO_PE11,
    PE12 = GPIO_PE12,
    PE13 = GPIO_PE13,
    PE14 = GPIO_PE14,
    PE15 = GPIO_PE15,
    PE16 = GPIO_PE16,
    PE17 = GPIO_PE17,

    PF0 = GPIO_PF0,
    PF1 = GPIO_PF1,
    PF2 = GPIO_PF2,
    PF3 = GPIO_PF3,
    PF4 = GPIO_PF4,
    PF5 = GPIO_PF5,
    PF6 = GPIO_PF6,
    
    PG0 = GPIO_PG0,
    PG1 = GPIO_PG1,
    PG2 = GPIO_PG2,
    PG3 = GPIO_PG3,
    PG4 = GPIO_PG4,
    PG5 = GPIO_PG5,
    PG6 = GPIO_PG6,
    PG7 = GPIO_PG7,
    PG8 = GPIO_PG8,
    PG9 = GPIO_PG9,
    PG10 = GPIO_PG10,
    PG11 = GPIO_PG11,
    PG12 = GPIO_PG12,
    PG13 = GPIO_PG13,
    PG14 = GPIO_PG14,
    PG15 = GPIO_PG15,

    /* To aviod compile warnings. */
    // GPIO_MAX
} pin_name2_t;

// FIXME:
typedef enum {
    PA0         = 0U,
    PA1,
    PA2,
    PA3,
    PA4,
    PA5,
    PA6,
    PA7,
    PA8,
    PA9,
    PA10,
    PA11,
    PA12,
    PA13,
    PA14,
    PA15,
    PA16,
    PA17,
    PA18,
    PA19,
    PA20,
    PA21,
    PA22,
    PA23,
    PA24,
    PA25,
    PA26,
    PA27,
    PA28,
    PA29,
    PA30,
} pin_name3_t;

typedef enum {
    PA0_VAD_DIN0                   =  0U,
    PA0_VAD_PDM_DIN0               =  1U,
    PA0_SPDIF0_DOUT                =  2U,
    PA0_I2S_IN_SDA2                =  3U,
    PA1_VAD_DIN1                   =  0U,
    PA1_VAD_PDM_DIN1               =  1U,
    PA1_SPDIF0_DIN                 =  2U,
    PA1_I2S_IN_SDA3                =  3U,
    PA2_VAD_WS                     =  0U,
    PA2_SPDIF1_DOUT                =  2U,
    PA2_I2S_IN_WSCLK               =  3U,
    PA3_VAD_SCLK                   =  0U,
    PA3_VAD_PDM_CLK                =  1U,
    PA3_SPDIF1_DIN                 =  2U,
    PA3_I2S_IN_SCLK                =  3U,
    PA4_VAD_DIN2                   =  0U,
    PA4_VAD_PDM_DIN2               =  1U,
    PA4_UART_RX                    =  2U,
    PA4_I2S_IN_SDA0                =  3U,
    PA5_VAD_DIN3                   =  0U,
    PA5_VAD_PDM_DIN3               =  1U,
    PA5_UART_TX                    =  2U,
    PA5_I2S_IN_SDA1                =  3U,
    PA6_IIC0_SCL                   =  0U,
    PA6_T_CPU_CLK                  =  1U,
    PA6_IIC1_SCL                   =  2U,
    PA6_UART_RX                    =  3U,
    PA7_IIC0_SDA                   =  0U,
    PA7_T_AXI_CLK                  =  1U,
    PA7_IIC1_SDA                   =  2U,
    PA7_UART_TX                    =  3U,
    PA8_CLK_12M                    =  0U,
    PA8_TAHB_CLK                   =  1U,
    PA8_VAD_MCLK                   =  2U,
    PA8_I2S_IN_MCLK                =  3U,
    PA9_I2S0_WSCLK                 =  0U,
    PA9_T_APB_CLK                  =  1U,
    PA9_TDM_DAT                    =  2U,
    PA9_I2S1_WSCLK                 =  3U,
    PA10_I2S0_SCLK                 =  0U,
    PA10_T_UART_CLK                =  1U,
    PA10_TDM_SCLK                  =  2U,
    PA10_I2S1_SCLK                 =  3U,
    PA11_I2S0_SDA                  =  0U,
    PA11_T_AUDIO_CLK               =  1U,
    PA11_TDM_WSCLK                 =  2U,
    PA11_I2S1_DIN                  =  3U,
    PA12_I2S0_MCLK                 =  0U,
    PA12_IIC1_SCL                  =  1U,
    PA12_UART_TX                   =  2U,
    PA12_I2S1_DOUT                 =  3U,
    PA13_I2S1_MCLK                 =  0U,
    PA13_IIC1_SDA                  =  1U,
    PA13_VAD_MCLK                  =  2U,
    PA13_UART_RX                   =  3U,
    PA14_I2S1_WSCLK                =  0U,
    PA14_VAD_PDM_DIN0              =  1U,
    PA14_VAD_DIN0                  =  2U,
    PA14_I2S0_WSCLK                =  3U,
    PA15_I2S1_SCLK                 =  0U,
    PA15_VAD_PDM_DIN1              =  1U,
    PA15_VAD_DIN1                  =  2U,
    PA15_CLK_12M                   =  3U,
    PA16_I2S1_SDI                  =  0U,
    PA16_VAD_PDM_CLK               =  1U,
    PA16_VAD_WS                    =  2U,
    PA16_IIC1_SCL                  =  3U,
    PA17_I2S1_SDO                  =  0U,
    PA17_VAD_PDM_DIN2              =  1U,
    PA17_VAD_SCLK                  =  2U,
    PA17_IIC1_SDA                  =  3U,
    PA18_I2S2_WSCLK                =  0U,
    PA18_TDM_DAT                   =  1U,
    PA18_VAD_DIN2                  =  2U,
    PA18_UART_CTS                  =  3U,
    PA19_I2S2_SCLK                 =  0U,
    PA19_TDM_SCLK                  =  1U,
    PA19_VAD_DIN3                  =  2U,
    PA19_UART_RTS                  =  3U,
    PA20_I2S2_MCLK                 =  0U,
    PA20_TDM_WSCLK                 =  1U,
    PA20_IIC1_SCL                  =  2U,
    PA20_IIC1_SDA                  =  3U,
    PA21_I2S2_SDI                  =  0U,
    PA21_SPDIF0_DIN                =  1U,
    PA21_IIC1_SDA                  =  2U,
    PA21_IIC1_SCL                  =  3U,
    PA22_I2S2_SDO                  =  0U,
    PA22_SPDIF0_DOUT               =  1U,
    PA22_CLK_12M                   =  2U,
    PA23_UART_RX                   =  0U,
    PA23_SPDIF1_DIN                =  1U,
    PA23_SPDIF0_DIN                =  2U,
    PA23_IIC1_SCL                  =  3U,
    PA24_UART_TX                   =  0U,
    PA24_SPDIF1_DOUT               =  1U,
    PA24_SPDIF0_DOUT               =  2U,
    PA24_IIC1_SDA                  =  3U,
    PA25_I2S_IN_DIN2               =  0U,
    PA25_UART_RTS                  =  1U,
    PA25_SPDIF0_DIN                =  2U,
    PA26_I2S_IN_DIN3               =  0U,
    PA26_UART_CTS                  =  1U,
    PA26_SPDIF0_DOUT               =  2U,
    PA26_CLK_12M                   =  3U,
    PA27_I2S_IN_WSCLK              =  0U,
    PA27_TDM_WSCLK                 =  1U,
    PA27_SPDIF1_DIN                =  2U,
    PA27_I2S0_WSCLK                =  3U,
    PA28_I2S_IN_SCLK               =  0U,
    PA28_TDM_SCLK                  =  1U,
    PA28_SPDIF1_DOUT               =  2U,
    PA28_I2S0_SCLK                 =  3U,
    PA29_I2S_IN_DIN0               =  0U,
    PA29_TDM_DAT                   =  1U,
    PA29_IIC0_SCL                  =  2U,
    PA29_I2S0_SDA                  =  3U,
    PA30_I2S_IN_DIN1               =  0U,
    PA30_IIC0_SDA                  =  2U,
    PA30_I2S0_MCLK                 =  3U,
    PIN_FUNC_GPIO                  =  4U,
} pin_func_t;

#define CONFIG_GPIO_NUM             2
#define CONFIG_IRQ_NUM              1023U
#define CONFIG_DMA_NUM              1


#define DW_TIMER0_BASE              0xFFCB011000UL
#define DW_TIMER0_SIZE              0x14U

#define DW_TIMER1_BASE              (DW_TIMER0_BASE+DW_TIMER0_SIZE)
#define DW_TIMER1_SIZE              DW_TIMER0_SIZE

#define DW_TIMER2_BASE              (DW_TIMER1_BASE+DW_TIMER1_SIZE)
#define DW_TIMER2_SIZE              DW_TIMER1_SIZE

#define DW_TIMER3_BASE              (DW_TIMER2_BASE+DW_TIMER2_SIZE)
#define DW_TIMER3_SIZE              DW_TIMER2_SIZE

#define DW_UART_BASE                0x02500000UL
#define DW_UART_SIZE                0x1000U

#define DW_GPIO_BASE                0xFFCB013000UL
#define DW_GPIO_SIZE                0x1000U

#define DW_WDT_BASE                 0xFFCB010000UL
#define DW_WDT_BASE_SZIE            0x1000U

#define DW_DMA_BASE                 0xFFC8000000UL
#define DW_DMA_BASE_SZIE            0x1000U

#define DW_BUS_MONITOR_BASE         0xFFCB020000UL
#define DW_BUS_MONITOR_BASE_SZIE    0x1000U

#define DW_MAILBOX_BASE             0xFFCA000000UL
#define DW_MAILBOX_BASE_SZIE        0x1000U

#define WJ_TDM_BASE                 0xFFCB012000UL
#define WJ_TDM_BASE_SZIE            0x1000U

#define WJ_I2S0_BASE                0xFFCB014000UL
#define WJ_I2S0_SIZE                0x1000U
#define WJ_I2S1_BASE                0xFFCB015000UL
#define WJ_I2S1_SIZE                0x1000U
#define WJ_I2S2_BASE                0xFFCB016000UL
#define WJ_I2S2_SIZE                0x1000U
#define WJ_I2S3_BASE                0xFFCB017000UL
#define WJ_I2S3_SIZE                0x1000U

#define WJ_SPDIF0_BASE              0xFFCB018000UL
#define WJ_SPDIF0_SIZE              0x1000U
#define WJ_SPDIF1_BASE              0xFFCB019000UL
#define WJ_SPDIF1_SIZE              0x1000U

#define DW_IIC0_BASE                0xFFCB01A000UL
#define DW_IIC0_BASE_SIZE           0x1000U

#define DW_IIC1_BASE                0xFFCB01B000UL
#define DW_IIC1_BASE_SIZE           0x1000U

#define WJ_IOC_BASE                 0xFFCB01D000UL
#define WJ_IOC_SIZE                 0x1000U

#define WJ_VAD_BASE                 0xFFCB01E000UL
#define WJ_VAD_BASE_SIZE            0x1000U

#define WJ_CPR_BASE                 0xFFCB000000UL
#define WJ_CPR_BASE_SIZE            0x10000U

#define WJ_MBOX_BASE                0xFFEFC48000UL
#define WJ_MBOX_SIZE                0x4000U

#define CONFIG_RTC_FAMILY_D

#define CONFIG_DW_AXI_DMA_32CH_NUM_CHANNELS

#define PLIC_BASE           (0x10000000UL)
#define CORET_BASE          (PLIC_BASE + 0x4000000UL)               /*!< CORET Base Address */
#define PLIC                ((PLIC_Type *)PLIC_BASE)

/* ================================================================================ */
/* ================                  otp declaration               ================ */
/* ================================================================================ */
#define CONFIG_OTP_BASE_ADDR 0U
#define CONFIG_OTP_BANK_SIZE 256U    // Bytes

#ifdef __cplusplus
}
#endif

#endif  /* _SOC_H_ */
