/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file       sys_clk.h
 * @brief      Header File for setting system frequency.
 * @version    V1.0
 * @date       14. Oct 2020
 * @model      sys_clk
 ******************************************************************************/

#ifndef _SYS_CLK_H_
#define _SYS_CLK_H_

#include <stdint.h>
#include <soc.h>
#include <drv/clk.h>
#include <drv/porting.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DE0_CLK                  = 29U,
    BUS_DE0_CLK              = 30U,
    DI_CLK                   = 31U,
    BUS_DI_CLK               = 32U,
    G2D_CLK                  = 33U,
    BUS_G2D_CLK              = 34U,
    CE_CLK                   = 35U,
    BUS_CE_CLK               = 36U,
    BUS_DMA_CLK              = 39U,
    AVS_CLK                  = 45U,
    BUS_PWM_CLK              = 47U,
    MMC0_CLK                 = 57U,
    MMC1_CLK                 = 58U,
    MMC2_CLK                 = 59U,
    BUS_MMC0_CLK             = 60U,
    BUS_MMC1_CLK             = 61U,
    BUS_MMC2_CLK             = 62U,
    BUS_UART0_CLK            = 63U,
    BUS_UART1_CLK            = 64U,
    BUS_UART2_CLK            = 65U,
    BUS_UART3_CLK            = 66U,
    BUS_UART4_CLK            = 67U,
    BUS_UART5_CLK            = 68U,
    BUS_I2C0_CLK             = 69U,
    BUS_I2C1_CLK             = 70U,
    BUS_I2C2_CLK             = 71U,
    BUS_I2C3_CLK             = 72U,
    SPI0_CLK                 = 75U,
    SPI1_CLK                 = 76U,
    BUS_SPI0_CLK             = 77U,
    BUS_SPI1_CLK             = 78U,
    I2S0_CLK                 = 85U,
    I2S1_CLK                 = 86U,
    I2S2_CLK                 = 87U,
    BUS_I2S0_CLK             = 89U,
    BUS_I2S1_CLK             = 90U,
    BUS_I2S2_CLK             = 91U,
    SPDIF_TX_CLK             = 92U,
    SPDIF_RX_CLK             = 93U,
    BUS_SPDIF_CLK            = 94U,
    DMIC_CLK                 = 95U,
    BUS_DMIC_CLK             = 96U,
    AUDIO_DAC_CLK            = 97U,
    AUDIO_ADC_CLK            = 98U,
    BUS_AUDIO_CODEC_CLK      = 99U,
    BUS_OTG_CLK              = 106U,
    BUS_LRADC_CLK            = 107U,
    MIPI_DSI_CLK             = 113U,
    BUS_MIPI_DSI_CLK         = 114U,
} clk_module_t;

void soc_clk_init(void);

uint32_t soc_get_sys_freq(void);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_CLK_H_ */
