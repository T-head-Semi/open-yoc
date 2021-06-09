/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     soc.h
 * @brief    CSI Core Peripheral Access Layer Header File
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

#ifndef EHS_VALUE
#define EHS_VALUE               20000000U
#endif

#ifndef ELS_VALUE
#define ELS_VALUE               32768U
#endif

#ifndef IHS_VALUE
#define IHS_VALUE               20000000U
#endif

#ifndef ILS_VALUE
#define ILS_VALUE               32768U
#endif

typedef enum {
    CMSDK_UARTRX0_IRQn             =  0U,
    CMSDK_UARTTX0_IRQn             =  1U,
    CORET_IRQn                     =  1U,
} irqn_type_t;

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
    PA31,
    PIN_END = 0xFFFFFFFFU
} pin_name_t;


typedef enum {
    PA0_I2S0_SCLK                  =  0U,
    PA0_SPI0_CS                    =  1U,
    PA0_UART0_RX                   =  2U,
    PA0_PWM_CH0                    =  3U,
    PA1_I2S0_WSCLK                 =  0U,
    PA1_SPI0_SCK                   =  1U,
    PA1_UART0_TX                   =  2U,
    PA1_PWM_CH1                    =  3U,
    PA2_I2S1_SCLK                  =  0U,
    PA2_IIC0_SCL                   =  1U,
    PA2_SPI1_CS                    =  2U,
    PA2_PWM_CH2                    =  3U,
    PA2_ADC_A0                     =  7U,
    PA3_I2S1_WSCLK                 =  0U,
    PA3_IIC0_SDA                   =  1U,
    PA3_SPI1_SCK                   =  2U,
    PA3_PWM_CH3                    =  3U,
    PA3_ADC_A1                     =  8U,
    PA4_I2S0_SDA                   =  0U,
    PA4_SPI0_MOSI                  =  1U,
    PA4_UART1_CTS                  =  2U,
    PA4_PWM_CH4                    =  3U,
    PA4_ADC_A2                     =  9U,
    PA5_I2S1_SDA                   =  0U,
    PA5_SPI0_MISO                  =  1U,
    PA5_UART1_RTS                  =  2U,
    PA5_PWM_CH5                    =  3U,
    PA5_ADC_A3                     = 10U,
    PA6_I2S0_SCLK                  =  0U,
    PA6_UART0_TX                   =  1U,
    PA6_SPI1_MOSI                  =  2U,
    PA6_PWM_CH6                    =  3U,
    PA6_ADC_A4                     = 11U,
    PA7_I2S0_WSCLK                 =  0U,
    PA7_PWMR_OUT                   =  1U,
    PA7_SPI1_MISO                  =  2U,
    PA7_PWM_CH7                    =  3U,
    PA7_ADC_A5                     = 12U,
    PA8_I2S0_SDA                   =  0U,
    PA8_IIC0_SCL                   =  1U,
    PA8_UART0_RX                   =  2U,
    PA8_PWM_CH8                    =  3U,
    PA8_ADC_A6                     = 13U,
    PA9_I2S1_SDA                   =  0U,
    PA9_IIC0_SDA                   =  1U,
    PA9_PWMR_OUT                   =  2U,
    PA9_PWM_CH9                    =  3U,
    PA9_ADC_A7                     = 14U,
    PA10_I2S0_MCLK                 =  0U,
    PA10_UART0_TX                  =  1U,
    PA10_SPI1_MOSI                 =  2U,
    PA10_SPI1_MISO                 =  3U,
    PA10_ADC_A8                    = 15U,
    PA15_IIC0_SCL                  =  0U,
    PA15_SPI0_CS                   =  1U,
    PA15_PWMR_OUT                  =  2U,
    PA15_PWM_CH4                   =  3U,
    PA15_ADC_A9                    = 20U,
    PA16_IIC0_SDA                  =  0U,
    PA16_SPI0_SCK                  =  1U,
    PA16_UART1_TX                  =  2U,
    PA16_PWM_CH5                   =  3U,
    PA16_ADC_A10                   = 21U,
    PA17_UART0_RX                  =  0U,
    PA17_SPI0_MOSI                 =  1U,
    PA17_I2S0_SCLK                 =  2U,
    PA17_PWM_CH10                  =  3U,
    PA17_ADC_A11                   = 22U,
    PA18_UART0_TX                  =  0U,
    PA18_SPI0_MISO                 =  1U,
    PA18_I2S0_WSCLK                =  2U,
    PA18_PWM_CH11                  =  3U,
    PA18_ADC_A12                   = 23U,
    PA19_JTAG_TMS                  =  0U,
    PA19_UART1_RX                  =  1U,
    PA19_I2S1_SCLK                 =  2U,
    PA19_IIC0_SCL                  =  3U,
    PA19_ADC_A13                   = 24U,
    PA20_JTAG_TCK                  =  0U,
    PA20_UART1_TX                  =  1U,
    PA20_I2S1_WSCLK                =  2U,
    PA20_IIC0_SDA                  =  3U,
    PA20_ADC_A14                   = 25U,
    PA21_UART0_CTS                 =  0U,
    PA21_UART1_CTS                 =  1U,
    PA21_I2S0_SDA                  =  2U,
    PA21_IIC0_SCL                  =  3U,
    PA21_ADC_A15                   = 26U,
    PA22_UART0_RTS                 =  0U,
    PA22_UART1_RTS                 =  1U,
    PA22_I2S1_SDA                  =  2U,
    PA22_IIC0_SDA                  =  3U,
    PA23_IIC0_SCL                  =  0U,
    PA23_UART0_TX                  =  1U,
    PA23_PWM_CH0                   =  2U,
    PA23_SPI0_CS                   =  3U,
    PA24_IIC0_SDA                  =  0U,
    PA24_UART0_RX                  =  1U,
    PA24_PWM_CH1                   =  2U,
    PA24_SPI0_SCK                  =  3U,
    PA25_PWMR_OUT                  =  0U,
    PA25_UART0_CTS                 =  1U,
    PA25_PWM_CH2                   =  2U,
    PA25_SPI0_MOSI                 =  3U,
    PA26_I2S1_MCLK                 =  0U,
    PA26_UART0_RTS                 =  1U,
    PA26_PWM_CH3                   =  2U,
    PA26_SPI0_MISO                 =  3U,
    PA27_I2S0_SCLK                 =  0U,
    PA27_UART1_RX                  =  1U,
    PA27_PWM_CH4                   =  2U,
    PA27_SPI1_CS                   =  3U,
    PA28_I2S0_WSCLK                =  0U,
    PA28_UART1_TX                  =  1U,
    PA28_PWM_CH5                   =  2U,
    PA28_SPI1_SCK                  =  3U,
    PA29_I2S1_SCLK                 =  0U,
    PA29_UART1_CTS                 =  1U,
    PA29_PWM_CH6                   =  2U,
    PA29_SPI1_MOSI                 =  3U,
    PA30_I2S1_WSCLK                =  0U,
    PA30_UART1_RTS                 =  1U,
    PA30_PWM_CH7                   =  2U,
    PA30_SPI1_MISO                 =  3U,
    PA31_I2S0_SDA                  =  0U,
    PA31_PWMR_OUT                  =  1U,
    PA31_PWM_CH8                   =  2U,
    PA31_UART0_TX                  =  3U,
    PIN_FUNC_GPIO                  =  4U,
    PIN_FUNC_END                   =  0xFFFFFFFFU
} pin_func_t;

#define CONFIG_IRQ_NUM              32U

#define SRAM_BASE                   0x20000000UL
#define SRAM_SIZE                   0x20000U

#define CMSDK_APB_BASE              (0x40000000UL)
#define CMSDK_UART0_BASE            (CMSDK_APB_BASE + 0x4000UL)

#ifdef __cplusplus
}
#endif

#endif  /* _SOC_H_ */
