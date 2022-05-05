/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <aos/aos.h>
#include <aos/kernel.h>
#include <aos/debug.h>
#include "aos/hal/uart.h"

static uart_dev_t _uart, *g_console_handle;

static uint16_t g_console_buf_size = 128;
static aos_mutex_t g_console_mutex_handle;
const char *console_get_devname(void)
{
    static char console_devname[32] = {0};

    if (g_console_handle) {
        snprintf(console_devname, sizeof(console_devname), "uart%d", g_console_handle->port);
    }

    return console_devname;
}

uint16_t console_get_buffer_size(void)
{
    return g_console_buf_size;
}

void console_init(int idx, uint32_t baud, uint16_t buf_size)
{
    int rc;
    aos_mutex_new(&g_console_mutex_handle);

    _uart.port                = idx;
    _uart.config.baud_rate    = baud;
    _uart.config.mode         = MODE_TX_RX;
    _uart.config.flow_control = FLOW_CONTROL_DISABLED;
    _uart.config.stop_bits    = STOP_BITS_1;
    _uart.config.parity       = NO_PARITY;
    _uart.config.data_width   = DATA_WIDTH_8BIT;

    rc = hal_uart_init(&_uart);
    if (rc == 0) {
        g_console_handle = &_uart;
        g_console_buf_size = buf_size;
    }
}

void console_deinit(void)
{
    //TODO:
    hal_uart_finalize(g_console_handle);
    g_console_handle = NULL;
    aos_mutex_free(&g_console_mutex_handle);
    return;
}

int uart_write(const void *buf, size_t size)
{
    CHECK_PARAM(g_console_handle, -1);
    if (!aos_irq_context()) {
        hal_uart_send(g_console_handle, buf, size, AOS_WAIT_FOREVER);
    } else {
        hal_uart_send_poll(g_console_handle, buf, size);
    }

    return size;
}

int uart_putc(int ch)
{
    CHECK_PARAM(g_console_handle, -1);
    if (ch == '\n') {
        int data = '\r';
        if (!aos_irq_context()) {
            hal_uart_send(g_console_handle, &data, 1, AOS_WAIT_FOREVER);
        } else {
            hal_uart_send_poll(g_console_handle, &data, 1);
        }
    }

    if (!aos_irq_context()) {
        hal_uart_send(g_console_handle, &ch, 1, AOS_WAIT_FOREVER);
    } else {
        hal_uart_send_poll(g_console_handle, &ch, 1);
    }

    return 0;
}

int uart_getc(void)
{
    if (g_console_handle != NULL) {
        char ch = 0;
        uint32_t rev_length = 0;

        hal_uart_recv_II(g_console_handle, &ch, 1, &rev_length, AOS_WAIT_FOREVER);

        return (int)ch;
    }

    return 0;
}

__attribute__((weak)) int os_critical_enter(unsigned int *lock)
{
    int ret;
    CHECK_PARAM(g_console_handle, -1);
    ret = aos_mutex_lock(&g_console_mutex_handle,10000);
    return ret;
}

__attribute__((weak)) int os_critical_exit(unsigned int *lock)
{
    int ret;
    CHECK_PARAM(g_console_handle, -1);
    ret = aos_mutex_unlock(&g_console_mutex_handle);
    return ret;
}
