/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _UART_DRV_H_
#define _UART_DRV_H_
#include <stdint.h>

int uart_csky_open(int uart_csi_id, int baud_rate);

int uart_csky_close(void);

int uart_csky_send(const void *data, uint32_t size);

int uart_csky_recv(void *data, uint32_t size);

#endif
