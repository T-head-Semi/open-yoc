/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     dw_spi.c
 * @brief
 * @version
 * @date     2020-06-22
 ******************************************************************************/

#include <string.h>
#include <drv/spi.h>
#include "sunxi_hal_spi.h"

csi_error_t csi_spi_init(csi_spi_t *spi, uint32_t idx)
{
    CSI_PARAM_CHK(spi, CSI_ERROR);
    hal_spi_master_port_t  port;
    hal_spi_master_config_t cfg;

    port = (hal_spi_master_port_t)idx;
    if (port >= HAL_SPI_MASTER_MAX) {
        return CSI_ERROR;
    }
    spi->dev.idx = idx;
    cfg.clock_frequency = 50000000;
    cfg.slave_port = HAL_SPI_MASTER_SLAVE_0;
    cfg.cpha = HAL_SPI_MASTER_CLOCK_PHASE0;
    cfg.cpol = HAL_SPI_MASTER_CLOCK_POLARITY0;
    spi_master_status_t status = aw_hal_spi_init(port, &cfg);
    if (status != SPI_MASTER_OK) {
        return CSI_ERROR;
    }

    return CSI_OK;
}

void csi_spi_uninit(csi_spi_t *spi)
{
    hal_spi_master_port_t port;
    CSI_PARAM_CHK_NORETVAL(spi);

    port = (hal_spi_master_port_t)spi->dev.idx;
    hal_spi_deinit(port);
}

csi_error_t csi_spi_attach_callback(csi_spi_t *spi, void *callback, void *arg)
{
    return CSI_UNSUPPORTED;
}

void csi_spi_detach_callback(csi_spi_t *spi)
{
}

csi_error_t csi_spi_mode(csi_spi_t *spi, csi_spi_mode_t mode)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_spi_cp_format(csi_spi_t *spi, csi_spi_cp_format_t format)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_spi_frame_len(csi_spi_t *spi, csi_spi_frame_len_t length)
{
    return CSI_UNSUPPORTED;
}

uint32_t csi_spi_baud(csi_spi_t *spi, uint32_t baud)
{
    spi_master_status_t status;
    hal_spi_master_port_t port;
    hal_spi_master_config_t cfg;
    CSI_PARAM_CHK(spi, CSI_ERROR);

    port = (hal_spi_master_port_t)spi->dev.idx;
    cfg.clock_frequency = baud;
    cfg.slave_port = HAL_SPI_MASTER_SLAVE_0;
    cfg.cpha = HAL_SPI_MASTER_CLOCK_PHASE0;
    cfg.cpol = HAL_SPI_MASTER_CLOCK_POLARITY0;
    status = hal_spi_hw_config(port, &cfg);
    if (status != SPI_MASTER_OK) {
    }
    return baud;
}

int32_t csi_spi_send(csi_spi_t *spi, const void *data, uint32_t size, uint32_t timeout)
{
    hal_spi_master_port_t port;
    spi_master_status_t status;
    CSI_PARAM_CHK(spi, CSI_ERROR);

    port = (hal_spi_master_port_t)spi->dev.idx;
    status = hal_spi_write(port, data, size);
    if (status != SPI_MASTER_OK) {
        return status;
    }
    return size;
}

csi_error_t csi_spi_send_async(csi_spi_t *spi, const void *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

int32_t csi_spi_receive(csi_spi_t *spi, void *data, uint32_t size, uint32_t timeout)
{
    hal_spi_master_port_t port;
    spi_master_status_t status;
    CSI_PARAM_CHK(spi, CSI_ERROR);
    port = (hal_spi_master_port_t)spi->dev.idx;
    status = hal_spi_read(port, data, size);
    if (status != SPI_MASTER_OK) {
        return status;
    }
    return size;
}

csi_error_t csi_spi_receive_async(csi_spi_t *spi, void *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

int32_t csi_spi_send_receive(csi_spi_t *spi, const void *data_out, void *data_in, uint32_t size, uint32_t timeout)
{
    return 0;
}

csi_error_t csi_spi_send_receive_async(csi_spi_t *spi, const void *data_out, void *data_in, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

void csi_spi_select_slave(csi_spi_t *spi, uint32_t slave_num)
{
}

csi_error_t csi_spi_link_dma(csi_spi_t *spi, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_spi_get_state(csi_spi_t *spi, csi_state_t *state)
{
    return CSI_UNSUPPORTED;
}
