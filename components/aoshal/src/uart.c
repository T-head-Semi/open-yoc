/*
 * Copyright (C) 2020-2021 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <aos/hal/uart.h>
#include <aos/kernel.h>
#include <errno.h>
#include <drv/uart.h>
#include <aos/ringbuffer.h>

#define HAL_UART_RINGBUF_LEN 256

#define EVENT_WRITE 0x0F0F0000
#define EVENT_READ  0x00000F0F

#define UART_RB_SIZE    256

typedef struct {
    csi_uart_t     handle;
    aos_event_t    event_write_read;
    char          *recv_buf;
    dev_ringbuf_t  read_buffer;
    uart_rx_cb     rx_cb;
    uart_dev_t    *uart_dev;
    aos_mutex_t    tx_mutex;
    aos_mutex_t    rx_mutex;
#ifdef UART_MODE_DMA
    csi_dma_ch_t g_dma_ch_tx;
    csi_dma_ch_t g_dma_ch_rx;
#endif
} hal_uart_priv_t;

static hal_uart_priv_t uart_list[6];

#ifndef UART_MODE_SYNC
static void uart_event_cb(csi_uart_t *uart, csi_uart_event_t event, void *arg)
{
    switch (event) {
    case UART_EVENT_SEND_COMPLETE:
        aos_event_set(&uart_list[(unsigned int)uart->dev.idx].event_write_read, EVENT_WRITE, AOS_EVENT_OR);
        break;
    case UART_EVENT_RECEIVE_COMPLETE: {
        aos_event_set(&uart_list[(unsigned int)uart->dev.idx].event_write_read, EVENT_READ, AOS_EVENT_OR);
        if (uart_list[(unsigned int)uart->dev.idx].rx_cb) {
            (*(uart_list[(unsigned int)uart->dev.idx].rx_cb))(uart_list[(unsigned int)uart->dev.idx].uart_dev);
        }
        break;
    };

    case UART_EVENT_RECEIVE_FIFO_READABLE: {
        int ret = 0;
        if (&uart_list[(unsigned int)uart->dev.idx].recv_buf != NULL) {
            uint8_t temp_buf[16] = {0};
            do {
                ret = csi_uart_receive(uart, temp_buf, 16, 0);
                if (ret > 0) {
                    if (ringbuffer_write(&uart_list[(unsigned int)uart->dev.idx].read_buffer, (uint8_t*)temp_buf, ret) != ret) {
                        break;
                    }
                }
            } while (ret);
        }
        aos_event_set(&uart_list[(unsigned int)uart->dev.idx].event_write_read, EVENT_READ, AOS_EVENT_OR);
        if (uart_list[(unsigned int)uart->dev.idx].rx_cb) {
            (*(uart_list[(unsigned int)uart->dev.idx].rx_cb))(uart_list[(unsigned int)uart->dev.idx].uart_dev);
        }
        break;
    }

    case UART_EVENT_ERROR_OVERFLOW: {
        int ret = 0;
        uint8_t temp_buf[16];
        do {
            ret = csi_uart_receive(uart, temp_buf, 16, 0);
        } while(ret);

        ringbuffer_clear(&uart_list[(unsigned int)uart->dev.idx].read_buffer);
        aos_event_set(&uart_list[(unsigned int)uart->dev.idx].event_write_read, ~EVENT_READ, AOS_EVENT_AND);

        break;
    }

    // case UART_EVENT_RECEIVE_FIFO_READABLE:
    //     csi_uart_receive(uart, tmp, 16, 0);
    //     break;

    default:
        break;
    }
}
#endif

int32_t hal_uart_init(uart_dev_t *uart)
{
    int32_t ret;

    if (uart == NULL) {
        return -1;
    }

    /* init uart */
    ret = csi_uart_init(&uart_list[uart->port].handle, uart->port);

    if (ret < 0) {
        return -1;
    }

#ifndef UART_MODE_SYNC
    ret = aos_event_new(&uart_list[uart->port].event_write_read, 0);

    if (ret != 0U) {
        return -1;
    }

#endif
    
    ret = aos_mutex_new(&uart_list[uart->port].tx_mutex);
    if (ret != 0) {
        return -1;
    }

    ret = aos_mutex_new(&uart_list[uart->port].rx_mutex);
    if (ret != 0) {
        return -1;
    }

    /* set uart baudrate */
    ret = csi_uart_baud(&uart_list[uart->port].handle, uart->config.baud_rate);

    if (ret < 0) {
        return -1;
    }

    /* set uart format */
    ret = csi_uart_format(&uart_list[uart->port].handle, uart->config.data_width, uart->config.parity, uart->config.stop_bits);

    if (ret < 0) {
        return -1;
    }

#ifndef UART_MODE_SYNC
    /* attach callback to uart device, the type should be the same with csi_uart_callback_t */
    ret = csi_uart_attach_callback(&uart_list[uart->port].handle, uart_event_cb, NULL);

    if (ret < 0) {
        return -1;
    }

    uart_list[uart->port].recv_buf = (char *)aos_malloc(UART_RB_SIZE);

    if (uart_list[uart->port].recv_buf == NULL) {
        return -1;
    }

    ringbuffer_create(&uart_list[uart->port].read_buffer, uart_list[uart->port].recv_buf, UART_RB_SIZE);

#endif

#ifdef UART_MODE_DMA
    ret = csi_uart_link_dma(&uart_list[uart->port].handle, &uart_list[uart->port].g_dma_ch_tx, &uart_list[uart->port].g_dma_ch_rx);

    if (ret < 0) {
        return -1;
    }

#endif
    
    uart_list[uart->port].uart_dev = uart;
    
    return 0;
}

int32_t hal_uart_send_poll(uart_dev_t *uart, const void *data, uint32_t size)
{
    uint32_t trans_num = 0U;
    uint8_t *ch = (uint8_t *)data;

    if (uart == NULL) {
        return -1;
    }

    while (trans_num < size) {
        csi_uart_putc(&uart_list[uart->port].handle, *ch++);
        trans_num++;
    }

    return trans_num;
}

int32_t hal_uart_send(uart_dev_t *uart, const void *data, uint32_t size, uint32_t timeout)
{
    int32_t ret = 0;
    unsigned int actl_flags = 0;

    if (uart == NULL) {
        return -1;
    }

    aos_mutex_lock(&uart_list[uart->port].tx_mutex, AOS_WAIT_FOREVER);
#ifdef UART_MODE_SYNC
    int32_t num;
    num = csi_uart_send(&uart_list[uart->port].handle, data, size, timeout);

    if (num != size) {
        return -1;
    }
#else
    ret = csi_uart_send_async(&uart_list[uart->port].handle, data, size);

    if (ret < 0) {
        return -1;
    }

    ret = aos_event_get(&uart_list[uart->port].event_write_read, EVENT_WRITE, AOS_EVENT_OR_CLEAR, &actl_flags, timeout);
    if (ret != 0) {
        return -1;
    }
#endif
    aos_mutex_unlock(&uart_list[uart->port].tx_mutex);

    return ret;
}

int32_t hal_uart_recv(uart_dev_t *uart, void *data, uint32_t expect_size, uint32_t timeout)
{
    int32_t ret = 0;
    unsigned int actl_flags = 0;

    aos_mutex_lock(&uart_list[uart->port].rx_mutex, AOS_WAIT_FOREVER);
#ifdef UART_MODE_SYNC
    int32_t num;
    num = csi_uart_receive(&uart_list[uart->port].handle, data, expect_size, timeout);

    if (num != expect_size) {
        return -1;
    }

#else
    ret = csi_uart_receive_async(&uart_list[uart->port].handle, data, expect_size);

    if (ret < 0) {
        return -1;
    }
    aos_mutex_unlock(&uart_list[uart->port].rx_mutex);

    ret = aos_event_get(&uart_list[uart->port].event_write_read, EVENT_READ, AOS_EVENT_OR_CLEAR, &actl_flags, timeout);

    if (ret != 0U) {
        return -1;
    }

#endif

    return ret;
}

int32_t hal_uart_recv_poll(uart_dev_t *uart, void *data, uint32_t expect_size)
{
    if (uart == NULL) {
        return -1;
    }

    uint8_t *temp_data = (uint8_t *)data;
    uint32_t recv_num = 0U;

    while (recv_num < expect_size) {
        *temp_data = csi_uart_getc(&uart_list[uart->port].handle);
        recv_num++;
        temp_data++;
    }

    return recv_num;
}

int32_t hal_uart_recv_II(uart_dev_t *uart, void *data, uint32_t expect_size,
                         uint32_t *recv_size, uint32_t timeout)
{
    unsigned int actl_flags = 0;
    int32_t      ret = 0;
    long long    time_enter, used_time;
    void *       temp_buf   = data;
    uint32_t     temp_count = expect_size;
    time_enter              = aos_now_ms();

    aos_mutex_lock(&uart_list[uart->port].rx_mutex, AOS_WAIT_FOREVER);
    while (1) {
        if (uart_list[uart->port].recv_buf != NULL) {
            ret = ringbuffer_read(&uart_list[uart->port].read_buffer, (uint8_t *)temp_buf, temp_count);
        } else {
            ret = csi_uart_receive(&uart_list[uart->port].handle, (uint8_t *)temp_buf, temp_count, 0);
        }
        temp_count = temp_count - ret;
        temp_buf   = (uint8_t *)temp_buf + ret;
        used_time  = aos_now_ms() - time_enter;

        if (timeout <= used_time || temp_count == 0 || timeout == 0) {
            break;
        }

        if (aos_event_get(&uart_list[uart->port].event_write_read, EVENT_READ, AOS_EVENT_OR_CLEAR, &actl_flags,
                          timeout - used_time) == -1) {
            break;
        }
    }
    *recv_size = expect_size - temp_count;
    aos_mutex_unlock(&uart_list[uart->port].rx_mutex);

    return *recv_size == expect_size ? 0 : -1;
}

int32_t hal_uart_recv_cb_reg(uart_dev_t *uart, uart_rx_cb cb)
{
    uart_list[uart->port].rx_cb = cb;
    return 0;
}

int32_t hal_uart_finalize(uart_dev_t *uart)
{
    aos_mutex_free(&uart_list[uart->port].tx_mutex);
    aos_mutex_free(&uart_list[uart->port].rx_mutex);
#ifdef UART_MODE_DMA
    csi_uart_link_dma(&uart_list[uart->port].handle, NULL, NULL);
#endif
#ifndef UART_MODE_SYNC
    aos_event_free(&uart_list[uart->port].event_write_read);
    aos_freep(&uart_list[uart->port].recv_buf);
#endif
    csi_uart_uninit(&uart_list[uart->port].handle);
    uart_list[uart->port].rx_cb = NULL;
    return 0;
}
