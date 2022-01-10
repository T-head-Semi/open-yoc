/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <aos/hal/spi.h>
#include <aos/kernel.h>
#include <errno.h>
#include <drv/spi.h>

/* #define SPI_MODE_SYNC */
/* #define SPI_MODE_DMA */
/*#define SPI_MODE_INTR*/

typedef struct {
#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
    aos_sem_t    spi_sem;
#endif
    csi_spi_t    handle;
#ifdef SPI_MODE_DMA
    csi_dma_ch_t dma_tx_ch;
    csi_dma_ch_t dma_rx_ch;
#endif
} hal_spi_priv_t;

static hal_spi_priv_t spi_list[6];

#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
static void spi_event_cb_fun(csi_spi_t *handle, csi_spi_event_t event, void *arg)
{
    spi_dev_t *spi;

    spi = (spi_dev_t *)arg;

    switch (event) {
        case SPI_EVENT_SEND_COMPLETE:
        case SPI_EVENT_RECEIVE_COMPLETE:
        case SPI_EVENT_SEND_RECEIVE_COMPLETE:
            aos_sem_signal(&spi_list[spi->port].spi_sem);
            break;

        default:
            ;
    }
}
#endif

int32_t hal_spi_init(spi_dev_t *spi)
{
    int32_t ret;

    if (spi == NULL) {
        return -1;
    }

    ret = csi_spi_init(&spi_list[spi->port].handle, spi->port);

    if (ret < 0) {
        return -1;
    }

#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
    ret = csi_spi_attach_callback(&spi_list[spi->port].handle, spi_event_cb_fun, spi);

    if (ret < 0) {
        return -1;
    }

#endif

#ifdef SPI_MODE_DMA
    ret = csi_spi_link_dma(&spi_list[spi->port].handle, &spi_list[spi->port].dma_tx_ch, &spi_list[spi->port].dma_rx_ch);

    if (ret < 0) {
        return -1;
    }

#endif

    if(spi->config.mode == HAL_SPI_MODE_MASTER){

        ret = csi_spi_mode(&spi_list[spi->port].handle, SPI_MASTER);

        if (ret < 0) {
            return -1;
        }
    } else if(spi->config.mode == HAL_SPI_MODE_SLAVE){

        ret = csi_spi_mode(&spi_list[spi->port].handle, SPI_SLAVE);

        if (ret < 0) {
            return -1;
        }
    }

    csi_spi_select_slave(&spi_list[spi->port].handle, 0); 

    ret = csi_spi_baud(&spi_list[spi->port].handle, spi->config.freq);

    if (ret < 0) {
        return -1;
    }

    ret = csi_spi_cp_format(&spi_list[spi->port].handle, SPI_FORMAT_CPOL0_CPHA0);

    if (ret < 0) {
        return -1;
    }

    ret = csi_spi_frame_len(&spi_list[spi->port].handle, SPI_FRAME_LEN_8);

    if (ret < 0) {
        return -1;
    }

#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
    ret = aos_sem_new(&spi_list[spi->port].spi_sem, 0);

    if (ret != 0U) {
        return -1;
    }

#endif

    return 0;
}

int32_t hal_spi_send(spi_dev_t *spi, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    int32_t ret;

    if (spi == NULL) {
        return -1;
    }

#ifdef SPI_MODE_SYNC
    ret = csi_spi_send(&spi_list[spi->port].handle, data, size, timeout);

    if (ret < 0) {
        return -1;
    }

#endif

#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
    ret = csi_spi_send_async(&spi_list[spi->port].handle, data, size);

    if (ret < 0) {
        return -1;
    }

    ret = aos_sem_wait(&spi_list[spi->port].spi_sem, timeout);

    if (ret != 0U) {
        return -1;
    }

#endif

    return 0;
}

int32_t hal_spi_recv(spi_dev_t *spi, uint8_t *data, uint16_t size, uint32_t timeout)
{
    int32_t ret;

    if (spi == NULL) {
        return -1;
    }

#ifdef SPI_MODE_SYNC
    ret = csi_spi_receive(&spi_list[spi->port].handle, data, size, timeout);

    if (ret < 0) {
        return -1;
    }

#endif

#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
    ret = csi_spi_receive_async(&spi_list[spi->port].handle, data, size);

    if (ret < 0) {
        return -1;
    }

    ret = aos_sem_wait(&spi_list[spi->port].spi_sem, timeout);

    if (ret != 0U) {
        return -1;
    }

#endif

    return 0;
}

int32_t hal_spi_send_recv(spi_dev_t *spi, uint8_t *tx_data, uint8_t *rx_data,
                          uint16_t size, uint32_t timeout)
{
    int32_t ret;

    if (spi == NULL) {
        return -1;
    }

#ifdef SPI_MODE_SYNC
    ret = csi_spi_send_receive(&spi_list[spi->port].handle, tx_data, rx_data, size, timeout);

    if (ret < 0) {
        return -1;
    }

#endif

#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
    ret = csi_spi_send_receive_async(&spi_list[spi->port].handle, tx_data, rx_data, size);

    if (ret < 0) {
        return -1;
    }

    ret = aos_sem_wait(&spi_list[spi->port].spi_sem, timeout);

    if (ret != 0U) {
        return -1;
    }

#endif

    return 0;
}

int32_t hal_spi_finalize(spi_dev_t *spi)
{

#ifndef SPI_MODE_SYNC //defined(SPI_MODE_DMA) || defined(SPI_MODE_INTR)
    aos_sem_free(&spi_list[spi->port].spi_sem);
#endif

#ifdef SPI_MODE_DMA
    csi_spi_link_dma(&spi_list[spi->port].handle, NULL, NULL);
#endif
    csi_spi_uninit(&spi_list[spi->port].handle);

    return 0;
}
