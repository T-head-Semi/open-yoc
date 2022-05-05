/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     devices.c
 * @brief    source file for the devices
 * @version  V1.0
 * @date     2019-12-18
******************************************************************************/

#include <stdio.h>
#include <csi_config.h>
#include <soc.h>
#include <drv/uart.h>
#include <drv/timer.h>
#include <drv/dma.h>
#include <drv/iic.h>
#include <drv/gpio.h>
#include <drv/irq.h>
#include <drv/pin.h>
#include <drv/i2s.h>

const csi_perip_info_t g_soc_info[] = {
    {DW_UART0_BASE,            DW_UART0_IRQn,            0,    DEV_DW_UART_TAG},
    {DW_UART1_BASE,            DW_UART1_IRQn,            1,    DEV_DW_UART_TAG},
    {0, 0, 0, 0}
};

const uint8_t g_dma_chnum[] = {16};

/* DMA handshake number */
/* The member of uart_tx_hs_num is the handshake number for ETB */
const uint16_t uart_tx_hs_num[] = {19};
const uint16_t uart_rx_hs_num[] = {18};
const uint16_t iic_tx_hs_num[]  = {21, 23};
const uint16_t iic_rx_hs_num[]  = {20, 22};
const uint16_t i2s_tx_hs_num[]  = {9, 11, 13, 36, 37, 38, 39};
const uint16_t i2s_rx_hs_num[]  = {8, 10, 12, 14, 15, 16, 17};
const uint16_t spdif_tx_hs_num[]  = {25, 27};
const uint16_t spdif_rx_hs_num[]  = {24, 26};
const uint16_t tdm_rx_hs_num[]  = {28, 29, 30, 31, 32, 33, 34, 35};
const uint16_t vad_rx_hs_num[]  = {0, 1, 2, 3, 4, 5, 6, 7};

const csi_dma_ch_desc_t uart0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t iic0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t iic1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s2_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s3_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t spdif0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t spdif1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t tdm_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t vad_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};


const csi_dma_ch_spt_list_t dma_spt_list[] = {
    {DEV_DW_UART_TAG,   0, uart0_dma_ch_list},
    {DEV_DW_IIC_TAG,    0, iic0_dma_ch_list},
    {DEV_DW_IIC_TAG,    1, iic1_dma_ch_list},
    {DEV_WJ_I2S_TAG,    0, i2s0_dma_ch_list},
    {DEV_WJ_I2S_TAG,    1, i2s1_dma_ch_list},
    {DEV_WJ_I2S_TAG,    2, i2s2_dma_ch_list},
    {DEV_WJ_I2S_TAG,    3, i2s3_dma_ch_list},
    {DEV_WJ_SPDIF_TAG,  0, spdif0_dma_ch_list},
    {DEV_WJ_SPDIF_TAG,  1, spdif1_dma_ch_list},
    {DEV_WJ_TDM_TAG,    0, tdm_dma_ch_list},
    {DEV_WJ_VAD_TAG,    0, vad_dma_ch_list},
    {0xFFFFU,         0xFFU,         NULL},
};

const csi_pinmap_t gpio_pinmap[] = {
    {PB0,    0,  0,           PIN_FUNC_GPIO },
    {PB1,    0,  1,           PIN_FUNC_GPIO },
    {PB2,    0,  2,           PIN_FUNC_GPIO },
    {PB3,    0,  3,           PIN_FUNC_GPIO },
    {PB4,    0,  4,           PIN_FUNC_GPIO },
    {PB5,    0,  5,           PIN_FUNC_GPIO },
    {PB6,    0,  6,           PIN_FUNC_GPIO },
    {PB7,    0,  7,           PIN_FUNC_GPIO },
    {PB8 ,   0,  8,           PIN_FUNC_GPIO },
    {PB9 ,   0,  9,           PIN_FUNC_GPIO },
    {PB10,   0,  10,          PIN_FUNC_GPIO },
    {PB11,   0,  11,          PIN_FUNC_GPIO },
    {PB12,   0,  12,          PIN_FUNC_GPIO },
    {PC0,    0,  0,           PIN_FUNC_GPIO },
    {PC1,    0,  1,           PIN_FUNC_GPIO },
    {PC2,    0,  2,           PIN_FUNC_GPIO },
    {PC3,    0,  3,           PIN_FUNC_GPIO },
    {PC4,    0,  4,           PIN_FUNC_GPIO },
    {PC5,    0,  5,           PIN_FUNC_GPIO },
    {PC6,    0,  6,           PIN_FUNC_GPIO },
    {PC7,    0,  7,           PIN_FUNC_GPIO },
    {PD0,    0,  0,           PIN_FUNC_GPIO },
    {PD1,    0,  1,           PIN_FUNC_GPIO },
    {PD2,    0,  2,           PIN_FUNC_GPIO },
    {PD3,    0,  3,           PIN_FUNC_GPIO },
    {PD4,    0,  4,           PIN_FUNC_GPIO },
    {PD5,    0,  5,           PIN_FUNC_GPIO },
    {PD6,    0,  6,           PIN_FUNC_GPIO },
    {PD7,    0,  7,           PIN_FUNC_GPIO },
    {PD8,    0,  8,           PIN_FUNC_GPIO },
    {PD9,    0,  9,           PIN_FUNC_GPIO },
    {PD10,   0,  10,          PIN_FUNC_GPIO },
    {PD11,   0,  11,          PIN_FUNC_GPIO },
    {PD12,   0,  12,          PIN_FUNC_GPIO },
    {PD13,   0,  13,          PIN_FUNC_GPIO },
    {PD14,   0,  14,          PIN_FUNC_GPIO },
    {PD15,   0,  15,          PIN_FUNC_GPIO },
    {PD16,   0,  16,          PIN_FUNC_GPIO },
    {PD17,   0,  17,          PIN_FUNC_GPIO },
    {PD18,   0,  18,          PIN_FUNC_GPIO },
    {PD19,   0,  19,          PIN_FUNC_GPIO },
    {PD20,   0,  20,          PIN_FUNC_GPIO },
    {PD21,   0,  21,          PIN_FUNC_GPIO },
    {PD22,   0,  22,          PIN_FUNC_GPIO },
    {PE0 ,   0,  0 ,          PIN_FUNC_GPIO },
    {PE1 ,   0,  1 ,          PIN_FUNC_GPIO },
    {PE2 ,   0,  2 ,          PIN_FUNC_GPIO },
    {PE3 ,   0,  3 ,          PIN_FUNC_GPIO },
    {PE4 ,   0,  4 ,          PIN_FUNC_GPIO },
    {PE5 ,   0,  5 ,          PIN_FUNC_GPIO },
    {PE6 ,   0,  6 ,          PIN_FUNC_GPIO },
    {PE7 ,   0,  7 ,          PIN_FUNC_GPIO },
    {PE8 ,   0,  8 ,          PIN_FUNC_GPIO },
    {PE9 ,   0,  9 ,          PIN_FUNC_GPIO },
    {PE10,   0,  10,          PIN_FUNC_GPIO },
    {PE11,   0,  11,          PIN_FUNC_GPIO },
    {PE12,   0,  12,          PIN_FUNC_GPIO },
    {PE13,   0,  13,          PIN_FUNC_GPIO },
    {PE14,   0,  14,          PIN_FUNC_GPIO },
    {PE15,   0,  15,          PIN_FUNC_GPIO },
    {PE16,   0,  16,          PIN_FUNC_GPIO },
    {PE17,   0,  17,          PIN_FUNC_GPIO },
    {PF0 ,   0,  0 ,          PIN_FUNC_GPIO },
    {PF1 ,   0,  1 ,          PIN_FUNC_GPIO },
    {PF2 ,   0,  2 ,          PIN_FUNC_GPIO },
    {PF3 ,   0,  3 ,          PIN_FUNC_GPIO },
    {PF4 ,   0,  4 ,          PIN_FUNC_GPIO },
    {PF5 ,   0,  5 ,          PIN_FUNC_GPIO },
    {PF6 ,   0,  6 ,          PIN_FUNC_GPIO },
    {PG0 ,   0,  0 ,          PIN_FUNC_GPIO },
    {PG1 ,   0,  1 ,          PIN_FUNC_GPIO },
    {PG2 ,   0,  2 ,          PIN_FUNC_GPIO },
    {PG3 ,   0,  3 ,          PIN_FUNC_GPIO },
    {PG4 ,   0,  4 ,          PIN_FUNC_GPIO },
    {PG5 ,   0,  5 ,          PIN_FUNC_GPIO },
    {PG6 ,   0,  6 ,          PIN_FUNC_GPIO },
    {PG7 ,   0,  7 ,          PIN_FUNC_GPIO },
    {PG8 ,   0,  8 ,          PIN_FUNC_GPIO },
    {PG9 ,   0,  9 ,          PIN_FUNC_GPIO },
    {PG10,   0,  10,          PIN_FUNC_GPIO },
    {PG11,   0,  11,          PIN_FUNC_GPIO },
    {PG12,   0,  12,          PIN_FUNC_GPIO },
    {PG13,   0,  13,          PIN_FUNC_GPIO },
    {PG14,   0,  14,          PIN_FUNC_GPIO },
    {PG15,   0,  15,          PIN_FUNC_GPIO },
    {PG16,   0,  16,          PIN_FUNC_GPIO },
    {PG17,   0,  17,          PIN_FUNC_GPIO },
    {PG18,   0,  18,          PIN_FUNC_GPIO },
    {0xFFFFFFFFU,   0xFFU, 0xFFU,       0xFFFFFFFFU   },
};

const csi_pinmap_t uart_pinmap[] = {
    // {PA4,             0,     PIN_UART_RX,    PA4_UART_RX  },
    // {PA5,             0,     PIN_UART_TX,    PA5_UART_TX  },
    // {PA6,             0,     PIN_UART_RX,    PA6_UART_RX  },
    // {PA7,             0,     PIN_UART_TX,    PA7_UART_TX  },
    // {PA12,            0,     PIN_UART_RX,    PA12_UART_TX },
    // {PA13,            0,     PIN_UART_TX,    PA13_UART_RX },
    // {PA23,            0,     PIN_UART_RX,    PA23_UART_RX },
    // {PA24,            0,     PIN_UART_TX,    PA24_UART_TX },
    {0xFFFFFFFFU, 0xFFU, 0xFFU,      0xFFFFFFFFU  },
};

const csi_pinmap_t iic_pinmap[] = {
    {0xFFFFFFFFU, 0xFFU,       0xFFU,   0xFFFFFFFFU },
};

const csi_pinmap_t i2s_pinmap[] = {
    {0xFFFFFFFFU, 0xFFU, 0xFFU, 0xFFFFFFFFU  },
};
