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

#define WJ_CPR_REG_BASE            (wj_cpr_regs_t *)WJ_CPR_BASE

typedef enum {
    IHS_CLK       = 0U,        /* Internal high speed clock */
    EHS_CLK,                   /* External high speed clock */
    ILS_CLK,                   /* Internal low  speed clock */
    ELS_CLK,                   /* External low  speed clock */
    PLL_CLK                    /* PLL clock */
} clk_src_t;

typedef enum {
    CPU_300MHZ    = 300000000U,
    CPU_288MHZ    = 288000000U,
    CPU_276MHZ    = 276000000U,
    CPU_270MHZ    = 270000000U,
    CPU_264MHZ    = 264000000U,
    CPU_252MHZ    = 252000000U,
    CPU_245_76MHZ = 245760000U,
    CPU_240MHZ    = 240000000U,
    CPU_228MHZ    = 228000000U,
    CPU_216MHZ    = 216000000U,
    CPU_204MHZ    = 204000000U,
    CPU_192MHZ    = 192000000U,
    CPU_180MHZ    = 180000000U,
    CPU_168MHZ    = 168000000U,
    CPU_156MHZ    = 156000000U,
    CPU_144MHZ    = 144000000U,
    CPU_135MHZ    = 135000000U,
    CPU_132MHZ    = 132000000U,
    CPU_120MHZ    = 120000000U,
    CPU_108MHZ    = 108000000U,
    CPU_96MHZ     = 96000000U,
    CPU_84MHZ     = 84000000U,
    CPU_72MHZ     = 72000000U,
    CPU_60MHZ     = 60000000U,
    CPU_48MHZ     = 48000000U,
    CPU_36MHZ     = 36000000U,
    CPU_30MHZ     = 30000000U,
    CPU_24MHZ     = 24000000U,
    CPU_20MHZ     = 20000000U,
    CPU_10MHZ     = 10000000U,
} sys_freq_t;


/* Pllclkout : ( pllclkin / 2)*( FN + Frac/4096 ) */
typedef struct {

    uint32_t pll_is_used;          /* Pll is used */

    uint32_t pll_source;           /* Select pll input source clock */

    uint32_t pll_src_clk_divider;  /* Ratio between pll_srcclk clock and pll_clkin clock */

    uint32_t fn;                   /* Integer value of frequency division */

    uint32_t frac;                 /* Decimal value of frequency division */

} pll_config_t;


typedef struct {
    uint32_t system_clk;            /* System clock */

    // pll_config_t pll_config;        /* Pll config struct */

    uint32_t sys_clk_source;        /* Select sysclk source clock */

    uint32_t rtc_clk_source;        /* Select rtcclk source clock */

    uint32_t cpu_clk_divider;

    uint32_t sys_clk_divider;

    uint32_t ahb_clk_divider;

    uint32_t apb_clk_divider;

    uint32_t uart_clk_divider;

    uint32_t audio_clk_divider;

    uint32_t vad_clk_divider;

} system_clk_config_t;

typedef enum {
    CLK_DIV1 = 0U,
    CLK_DIV2,
    CLK_DIV3,
    CLK_DIV4,
    CLK_DIV5,
    CLK_DIV6,
    CLK_DIV7,
    CLK_DIV8,
    CLK_DIV9,
    CLK_DIV10,
    CLK_DIV11,
    CLK_DIV12,
    CLK_DIV13,
    CLK_DIV14,
    CLK_DIV15,
    CLK_DIV16
} apb_div_t;

typedef enum {
    PLL_FN_18 = 0U,
    PLL_FN_19,
    PLL_FN_20,
    PLL_FN_21,
    PLL_FN_22,
    PLL_FN_23,
    PLL_FN_24,
    PLL_FN_25,
    PLL_FN_26,
    PLL_FN_27,
    PLL_FN_28,
    PLL_FN_29,
    PLL_FN_30,
    PLL_FN_31,
    PLL_FN_32,
    PLL_FN_33,
    PLL_FN_34,
    PLL_FN_35,
    PLL_FN_36,
    PLL_FN_37,
    PLL_FN_38,
    PLL_FN_39,
    PLL_FN_40,
    PLL_FN_41,
    PLL_FN_42,
    PLL_FN_43,
    PLL_FN_44,
    PLL_FN_45,
    PLL_FN_46,
    PLL_FN_47,
    PLL_FN_48,
    PLL_FN_49
} pll_fn_t;

typedef enum {
    TIM0_CLK = 0U,
    TIM1_CLK,
    RTC0_CLK,
    WDT_CLK,
    SPI0_CLK,
    UART0_CLK,
    IIC0_CLK,
    PWM_CLK,
    QSPI0_CLK,
    PWMR_CLK,
    EFUSE_CLK,
    I2S0_CLK,
    I2S1_CLK,
    GPIO0_CLK,

    TIM2_CLK = 32U,
    TIM3_CLK,
    SPI1_CLK,
    UART1_CLK,
    I2S567_CLK,
    ADC_CLK,
    ETB_CLK,
    I2S2_CLK,
    I2S3_CLK,
    IOC_CLK,
    CODEC_CLK
} clk_module_t;

typedef enum {
    SRST_DMA = 0U,
    SRST_BSM,
    SRST_TIMER,
    SRST_TIM_CNT1,
    SRST_TIM_CNT2,
    SRST_TIM_CNT3,
    SRST_TIM_CNT4,
    SRST_WDR,
    SRST_USI0,
    SRST_USI1,
    SRST_UART,
    SRST_I2S0,
    SRST_I2S1,
    SRST_I2S2,
    SRST_I2SIN,
    SRST_TDM,
    SRST_GPIO,
    SRST_SPDIF0,
    SRST_SPDIF1,
    SRST_VAD,
    SRST_IOMUX,
    SRST_MAXIMUM
} clk_soft_reset_t;

typedef enum {
    CG_CPU = 0U,
    CG_SRAM0,
    CG_SRAM1,
    CG_DMA,
    CG_BSM,
    CG_TIMER,
    CG_TIM_CNT1,
    CG_TIM_CNT2,
    CG_TIM_CNT3,
    CG_TIM_CNT4,
    CG_WDR,
    CG_USI0,
    CG_USI1,
    CG_UART,
    CG_I2S0,
    CG_I2S1,
    CG_I2S2,
    CG_I2SIN,
    CG_TDM,
    CG_GPIO,
    CG_SPDIF0,
    CG_SPDIF1,
    CG_VAD,
    CG_MAXIMUM
} clk_clock_gate_t;


void soc_clk_init(void);

/**
  \brief       Set the system clock according to the parameter
  \param[in]   config    System clock config.
  \return      Error code \ref csi_error_t
*/
csi_error_t soc_sysclk_config(system_clk_config_t *config);

/**
  \brief       Set iic reset
  \param[in]   idx    iic idx.
  \return      None
*/
void soc_reset_iic(uint32_t idx);

/**
  \brief       Perform soft reset operation on related IP
  \param[in]   ip    Operation IP \ref clk_soft_reset_t
  \return      None
*/
void soc_soft_reset_single(clk_soft_reset_t ip);

/**
  \brief       Perform clock gate operation on related IP
  \param[in]   ip       Operation IP \ref clk_clock_gate_t
  \param[in]   state    IP control state(1:on 0:off)
  \return      None
*/
void soc_clock_gate_single(clk_clock_gate_t ip, uint32_t state);


uint32_t soc_get_pll_freq(void);
uint32_t soc_get_cur_cpu_freq(void);
uint32_t soc_get_coretim_freq(void);
uint32_t soc_get_ahb_freq(uint32_t idx);
uint32_t soc_get_apb_freq(uint32_t idx);
uint32_t soc_get_cpu_freq(uint32_t idx);
uint32_t soc_get_sys_freq(uint32_t idx);
uint32_t soc_get_audio_clk(uint32_t idx);

uint32_t soc_get_uart_freq(uint32_t idx);
uint32_t soc_get_iic_freq(uint32_t idx);
uint32_t soc_get_spi_freq(uint32_t idx);
uint32_t soc_get_qspi_freq(uint32_t idx);
uint32_t soc_get_adc_freq(uint32_t idx);
uint32_t soc_get_pwm_freq(uint32_t idx);
uint32_t soc_get_wdt_freq(uint32_t idx);
uint32_t soc_get_i2s_freq(uint32_t idx);
uint32_t soc_get_rtc_freq(uint32_t idx);
uint32_t soc_get_timer_freq(uint32_t idx);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_CLK_H_ */
