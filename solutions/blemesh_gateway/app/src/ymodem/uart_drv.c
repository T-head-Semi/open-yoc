/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>

#include <drv/usart.h>

#include "ringbuffer.h"
#include "uart_drv.h"

#define UART_RB_SIZE 16384

typedef struct {
    usart_handle_t handle;
    char           recv_buf[UART_RB_SIZE];
    dev_ringbuf_t  read_buffer;
} uart_dev_t;

static uart_dev_t g_uart_inst = {0};

static void usart_csky_event_cb_fun(int32_t uart_idx, usart_event_e event)
{
    uart_dev_t *uart = &g_uart_inst;

    switch (event) {
        case USART_EVENT_SEND_COMPLETE:
            break;

        case USART_EVENT_RECEIVED: {
            int32_t ret;
            if (uart->recv_buf != NULL) {
                char buffer[32];
                do {
                    ret = csi_usart_receive_query(uart->handle, buffer, sizeof(buffer));

                    if (ret > 0) {
                        if (ringbuffer_write(&uart->read_buffer, (uint8_t *)buffer, ret) != ret) {
                            break;
                        }
                    }
                } while (ret);
            }
            break;
        };

        case USART_EVENT_RX_OVERFLOW: {
            char     buffer[32];
            uint32_t ret;

            do {
                /* lost some data, clean hw buffer and ringbuffer */
                ret = csi_usart_receive_query(uart->handle, buffer, sizeof(buffer));
            } while (ret);

            if (uart->recv_buf != NULL) {
                ringbuffer_clear(&uart->read_buffer);
            }
            break;
        }

        case USART_EVENT_RX_FRAMING_ERROR:
        default:
            // LOGW(TAG, "uart%d event %d", idx, event);
            break;
    }
}

int uart_csky_open(int uart_csi_id, int baud_rate)
{
    uart_dev_t *uart = &g_uart_inst;

    ringbuffer_create(&uart->read_buffer, uart->recv_buf, UART_RB_SIZE);

    uart->handle = csi_usart_initialize(uart_csi_id, usart_csky_event_cb_fun);

    if (uart->handle) {
        csi_usart_config(uart->handle, baud_rate, USART_MODE_ASYNCHRONOUS, USART_PARITY_NONE,
                         USART_STOP_BITS_1, USART_DATA_BITS_8);
    }

    return uart->handle ? 0 : -1;
}

int uart_csky_close(void)
{
    uart_dev_t *uart = &g_uart_inst;

    csi_usart_uninitialize(uart->handle);

    return 0;
}

int uart_csky_send(const void *data, uint32_t size)
{
    uart_dev_t *uart = &g_uart_inst;

    int i;
    for (i = 0; i < size; i++) {
        //dont depend interrupt
        csi_usart_putchar(uart->handle, *((uint8_t *)data + i));
    }

    return 0;
}

int uart_csky_recv(void *data, uint32_t size)
{
    uart_dev_t *uart = &g_uart_inst;

    int ret = 0;

    ret = ringbuffer_read(&uart->read_buffer, (uint8_t *)data, size);

    return ret;
}
