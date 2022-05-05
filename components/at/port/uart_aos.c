#include <stdio.h>
#include <errno.h>

#include <aos/aos.h>
#include "aos/hal/uart.h"

#include "yoc/at_port.h"

typedef struct {
    uart_dev_t      dev;
    channel_event_t cb;
    void           *priv;
} aos_uart_t;

static int32_t uart_rx(uart_dev_t *uart_dev_t)
{
    aos_uart_t *uart = (aos_uart_t *)uart_dev_t;
    
    if (uart->cb)
        uart->cb(AT_CHANNEL_EVENT_READ, uart->priv);

    return 0;
}

void *at_uart_init(const char *name, void *config)
{
    int idx;
    int len = strlen(name);
    uart_config_t *uart_config = (uart_config_t*)config;
    aos_uart_t *uart = aos_zalloc_check(sizeof(aos_uart_t));

    idx = name[len-1] - 0x30;
    uart->dev.port                = idx;
    uart->dev.config.baud_rate    = uart_config->baud_rate;
    uart->dev.config.mode         = uart_config->mode;
    uart->dev.config.flow_control = uart_config->flow_control;
    uart->dev.config.stop_bits    = uart_config->stop_bits;
    uart->dev.config.parity       = uart_config->parity;
    uart->dev.config.data_width   = uart_config->data_width;

    int ret = hal_uart_init(&uart->dev);
    
    return (ret >= 0 ? uart : NULL); 
}

int at_uart_set_event(void *uart_hdl, channel_event_t evt_cb, void *priv)
{
    aos_uart_t *uart = (aos_uart_t *)uart_hdl;
    uart->cb   = evt_cb;
    uart->priv = priv;

    hal_uart_recv_cb_reg(&uart->dev, uart_rx);
    return 0;
}

int at_uart_send(void *uart_hdl, const char *data, int size)
{
    aos_uart_t *uart = (aos_uart_t *)uart_hdl;
    return(hal_uart_send(&uart->dev, (void *)data, size, AOS_WAIT_FOREVER));
}

int at_uart_recv(void *uart_hdl, const char *data, int size, int timeout)
{
    aos_uart_t *uart = (aos_uart_t *)uart_hdl;
    uint32_t recv_len = 0;

    hal_uart_recv_II(&uart->dev, (void *)data, size, &recv_len, timeout);

    return recv_len;
}

at_channel_t uart_channel = {
    .init       = at_uart_init,
    .set_event  = at_uart_set_event,
    .send       = at_uart_send,
    .recv       = at_uart_recv,
};
