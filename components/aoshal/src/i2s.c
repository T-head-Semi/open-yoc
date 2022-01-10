/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <aos/kernel.h>
#include "aos/hal/i2s.h"
#include "drv/i2s.h"
#include <drv/ringbuf.h>

#define I2S_BUF_SIZE 2048
#define I2S_CALLBACK_PERIOD 512

#define    I2S_MODE_DMA
// #define    I2S_MODE_SYNC
// #define    I2S_MODE_INTR

typedef struct {
    aos_sem_t rx_period_sem;
    aos_sem_t tx_period_sem;
    csi_i2s_t handle;
    csi_ringbuf_t i2s_tx_ringbuffer;
    csi_ringbuf_t i2s_rx_ringbuffer;
#ifdef I2S_MODE_DMA
    csi_dma_ch_t rx_dma_ch;
    csi_dma_ch_t tx_dma_ch;
#endif
} hal_i2s_priv_t;

static hal_i2s_priv_t i2s_list[6];

static void i2s_cb(csi_i2s_t *handle, csi_i2s_event_t event)
{
    if (event == I2S_EVENT_SEND_COMPLETE) {
        aos_sem_signal(&i2s_list[handle->dev.idx].tx_period_sem);
    } else if (event == I2S_EVENT_RECEIVE_COMPLETE) {
        aos_sem_signal(&i2s_list[handle->dev.idx].rx_period_sem);
    } else {
        printf("i2s err event %d\n", event);
    }
}

int32_t hal_i2s_init(i2s_dev_t *i2s)
{
    csi_error_t ret;
    csi_i2s_format_t i2s_format;

    if (i2s == NULL) {
        return -1;
    }

    if (aos_sem_new(&i2s_list[i2s->port].rx_period_sem, 1) != 0) {
        return -1;
    }

    if (aos_sem_new(&i2s_list[i2s->port].tx_period_sem, 1) != 0) {
        return -1;
    }

    ret = csi_i2s_init(&i2s_list[i2s->port].handle, i2s->port);

    if (ret != CSI_OK) {
        printf("csi_i2s_init error\n");
        return -1;
    }

#if defined(I2S_MODE_INTR) || defined(I2S_MODE_DMA)
    ret = csi_i2s_attach_callback(&i2s_list[i2s->port].handle, i2s_cb, NULL);

    if (ret != CSI_OK) {
        printf("csi_i2s_attach_callback error\n");
        return -1;
    }

#endif

#ifdef I2S_MODE_DMA

    if ((i2s->config.mode == MODE_MASTER_TX) || (i2s->config.mode == MODE_SLAVE_TX)) {
        ret = csi_i2s_rx_link_dma(&i2s_list[i2s->port].handle, &i2s_list[i2s->port].rx_dma_ch);

        if (ret != CSI_OK) {
            printf("csi_iic_link_dma fail \n");
            return -1;
        }
    }

    if ((i2s->config.mode == MODE_MASTER_RX) || (i2s->config.mode == MODE_SLAVE_RX)) {
        ret = csi_i2s_tx_link_dma(&i2s_list[i2s->port].handle, &i2s_list[i2s->port].tx_dma_ch);

        if (ret != CSI_OK) {
            printf("csi_iic_link_dma fail \n");
            return -1;
        }
    }

#endif

    if ((i2s->config.mode == MODE_MASTER_TX) || (i2s->config.mode == MODE_MASTER_RX)) {
        i2s_format.mode = I2S_MODE_MASTER;
    }

    if ((i2s->config.mode == MODE_SLAVE_TX) || (i2s->config.mode == MODE_SLAVE_RX)) {
        i2s_format.mode = I2S_MODE_SLAVE;
    }

    switch (i2s->config.standard) {
        case STANDARD_PHILIPS:
            i2s_format.width = I2S_SAMPLE_WIDTH_16BIT;
            break;

        case STANDARD_MSB:
            i2s_format.width = I2S_SAMPLE_WIDTH_16BIT;
            break;

        case STANDARD_LSB:
            i2s_format.width = I2S_SAMPLE_WIDTH_24BIT;
            break;

        case STANDARD_PCM_SHORT:
            i2s_format.width = I2S_SAMPLE_WIDTH_32BIT;
            break;

        case STANDARD_PCM_LONG:
            i2s_format.width = I2S_SAMPLE_WIDTH_32BIT;
            break;

        default:
            break;
    }

    switch (i2s->config.standard) {
        case STANDARD_PHILIPS:
            i2s_format.protocol = I2S_PROTOCOL_I2S;
            break;

        case STANDARD_MSB:
            i2s_format.protocol = I2S_PROTOCOL_MSB_JUSTIFIED;
            break;

        case STANDARD_LSB:
            i2s_format.protocol = I2S_PROTOCOL_LSB_JUSTIFIED;
            break;

        case STANDARD_PCM_SHORT:
            i2s_format.protocol = I2S_PROTOCOL_PCM;
            break;

        case STANDARD_PCM_LONG:
            i2s_format.protocol = I2S_PROTOCOL_PCM;
            break;

        default:
            break;
    }

    i2s_format.rate = i2s->config.freq;
    i2s_format.sclk_nfs = I2S_SCLK_32FS;
    i2s_format.mclk_nfs = I2S_MCLK_256FS;
    i2s_format.polarity = 0;
    csi_i2s_format(&i2s_list[i2s->port].handle, &i2s_format);

    if ((i2s->config.mode == MODE_MASTER_TX) || (i2s->config.mode == MODE_SLAVE_TX)) {
        csi_i2s_tx_link_dma(&i2s_list[i2s->port].handle, &i2s_list[i2s->port].tx_dma_ch);

        i2s_list[i2s->port].i2s_tx_ringbuffer.size = I2S_BUF_SIZE;
        i2s_list[i2s->port].i2s_tx_ringbuffer.buffer = (uint8_t *)aos_malloc(I2S_BUF_SIZE);
        csi_i2s_tx_set_buffer(&i2s_list[i2s->port].handle, &i2s_list[i2s->port].i2s_tx_ringbuffer);
        csi_i2s_tx_buffer_reset(&i2s_list[i2s->port].handle);
        ret = csi_i2s_tx_set_period(&i2s_list[i2s->port].handle, I2S_CALLBACK_PERIOD);
        csi_i2s_send_start(&i2s_list[i2s->port].handle);
    }

    if ((i2s->config.mode == MODE_MASTER_RX) || (i2s->config.mode == MODE_SLAVE_RX)) {
        csi_i2s_rx_link_dma(&i2s_list[i2s->port].handle, &i2s_list[i2s->port].rx_dma_ch);

        i2s_list[i2s->port].i2s_rx_ringbuffer.size = I2S_BUF_SIZE;
        i2s_list[i2s->port].i2s_rx_ringbuffer.buffer = (uint8_t *)aos_malloc(I2S_BUF_SIZE);
        csi_i2s_rx_set_buffer(&i2s_list[i2s->port].handle, &i2s_list[i2s->port].i2s_rx_ringbuffer);
        csi_i2s_rx_buffer_reset(&i2s_list[i2s->port].handle);
        ret = csi_i2s_rx_set_period(&i2s_list[i2s->port].handle, I2S_CALLBACK_PERIOD);
        csi_i2s_receive_start(&i2s_list[i2s->port].handle);
    }

    return  ret;
}

int32_t hal_i2s_send(i2s_dev_t *i2s, const void *data, uint32_t size, uint32_t timeout)
{
    int32_t num;

#ifdef I2S_MODE_SYNC
    num = csi_i2s_send(&i2s_list[i2s->port].handle, data, size, timeout);

    if (num != size) {
        printf("csi_i2s_master_send error\n");
        return -1;
    }

#endif
#if defined(I2S_MODE_DMA) || defined(I2S_MODE_INTR)
    num = csi_i2s_send_async(&i2s_list[i2s->port].handle, data, size);

    if (num != size) {
        printf("csi_i2s_master_send error\n");
        return -1;
    }

#endif
    return num;
}

int32_t hal_i2s_recv(i2s_dev_t *i2s, void *data, uint32_t size, uint32_t timeout)
{
    int32_t num;

    num = csi_i2s_receive_async(&i2s_list[i2s->port].handle, data, size);

    if (num != size) {
        printf("csi_i2s_master_send error\n");
        return -1;
    }

    return num;
}

int32_t hal_i2s_pause(i2s_dev_t *i2s)
{
    int32_t ret = 0;

    if ((i2s->config.mode == MODE_MASTER_TX) || (i2s->config.mode == MODE_SLAVE_TX)) {
        ret = csi_i2s_send_pause(&i2s_list[i2s->port].handle);
    }

    if ((i2s->config.mode == MODE_MASTER_RX) || (i2s->config.mode == MODE_SLAVE_RX)) {
        csi_i2s_receive_stop(&i2s_list[i2s->port].handle);
    }

    return ret;
}


int32_t hal_i2s_resume(i2s_dev_t *i2s)
{
    int32_t ret = 0;

    if ((i2s->config.mode == MODE_MASTER_TX) || (i2s->config.mode == MODE_SLAVE_TX)) {
        ret = csi_i2s_send_resume(&i2s_list[i2s->port].handle);
    }

    if ((i2s->config.mode == MODE_MASTER_RX) || (i2s->config.mode == MODE_SLAVE_RX)) {
        ret = csi_i2s_receive_start(&i2s_list[i2s->port].handle);
    }

    return ret;
}

int32_t hal_i2s_stop(i2s_dev_t *i2s)
{
    if ((i2s->config.mode == MODE_MASTER_TX) || (i2s->config.mode == MODE_SLAVE_TX)) {
        csi_i2s_send_stop(&i2s_list[i2s->port].handle);
    }

    if ((i2s->config.mode == MODE_MASTER_RX) || (i2s->config.mode == MODE_SLAVE_RX)) {
        csi_i2s_receive_stop(&i2s_list[i2s->port].handle);
    }

    return 0;
}


int32_t hal_i2s_finalize(i2s_dev_t *i2s)
{
    uint32_t ret;

    if ((i2s->config.mode == MODE_MASTER_TX) || (i2s->config.mode == MODE_SLAVE_TX)) {
        ret = csi_i2s_tx_link_dma(&i2s_list[i2s->port].handle, NULL);

        if (ret != CSI_OK) {
            printf("csi_iic_link_dma fail \n");
            return -1;
        }

        aos_free(&i2s_list[i2s->port].i2s_tx_ringbuffer);
    }

    if ((i2s->config.mode == MODE_MASTER_RX) || (i2s->config.mode == MODE_SLAVE_RX)) {
        ret = csi_i2s_rx_link_dma(&i2s_list[i2s->port].handle, NULL);

        if (ret != CSI_OK) {
            printf("csi_iic_link_dma fail \n");
            return -1;
        }

        aos_free(&i2s_list[i2s->port].i2s_rx_ringbuffer);
    }

    csi_i2s_detach_callback(&i2s_list[i2s->port].handle);
    csi_i2s_uninit(&i2s_list[i2s->port].handle);
    return ret;
}
