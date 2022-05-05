/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <pin_name.h>
#include "app_main.h"


typedef struct {
    int32_t    tx;
    int32_t    rx;
    uint16_t cfg_idx;
    uint32_t   tx_func;
    uint32_t   rx_func;
} usart_pinmap_t;
const static usart_pinmap_t g_usart_pinmap[] = {
    {
        PB19,
        PB20,
        0,
        PB19_UART0_TX,
        PB20_UART0_RX
    },
    {
        PB6,
        PB7,
        1,
        PB6_UART1_TX,
        PB7_UART1_RX
    },
};

void app_pinmap_usart_init(int32_t uart_idx)
{
    for(int i = 0; i < sizeof(g_usart_pinmap) / sizeof(usart_pinmap_t); i++) {
        if(g_usart_pinmap[i].cfg_idx == uart_idx) {
            drv_pinmux_config(g_usart_pinmap[uart_idx].tx, g_usart_pinmap[uart_idx].tx_func);
            drv_pinmux_config(g_usart_pinmap[uart_idx].rx, g_usart_pinmap[uart_idx].rx_func);
            break;
        }
    }
}