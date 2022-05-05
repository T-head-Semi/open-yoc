/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     spiflash.c
 * @brief
 * @version
 * @date     2020-06-23
 ******************************************************************************/
#include <string.h>
#include <drv/spiflash.h>
#include <drv/irq.h>
#include <drv/tick.h>
#include <sunxi_hal_spinor.h>

/**
  \brief       Initialize SPIFLASH with spi controler  and probe flash device
  \param[in]   spiflash   SPIFLASH handle
  \param[in]   spi_idx    SPI controler index
  \param[in]   spi_cs     GPIO info for chip select,if NULL, not use gpio cs
  \return      Error code
*/
csi_error_t csi_spiflash_qspi_init(csi_spiflash_t *spiflash, uint32_t spi_idx, void *spi_cs_callback)
{
    CSI_PARAM_CHK(spiflash, CSI_ERROR);
    // CSI_PARAM_CHK(spi_cs_callback, CSI_ERROR);

    // uint32_t addr, addr_size, data_size;
    // uint32_t data = 0U;
    // uint32_t flash_id = 0U;
    // uint32_t baud;
    // uint8_t cmd;

    // spiflash->spi_cs_callback = spi_cs_callback;
    // spiflash->spi_cs_callback(GPIO_PIN_HIGH);
    // spiflash->spi_send = send;
    // spiflash->spi_receive = receive;

    int32_t ret = hal_spinor_init(0);
    if (ret != 0)
    {
        return CSI_ERROR;
    }
    return CSI_OK;
}

/**
  \brief       De-initialize SPIFLASH Interface based on spi controler. stops operation and releases the software resources used by the interface
  \param[in]   spiflash    SPIFLASH handle to operate
  \return      Error code
*/
void csi_spiflash_qspi_uninit(csi_spiflash_t *spiflash)
{
    // CSI_PARAM_CHK(spiflash, CSI_ERROR);
    hal_spinor_deinit();
}

/**
  \brief       Get flash device infomation
  \param[in]   spiflash      SPIFLASH handle to operate
  \param[in]   flash_info    User storage to get flash vendor info after flash init
  \return      spiflash_info_t
*/
csi_error_t csi_spiflash_get_flash_info(csi_spiflash_t *spiflash, csi_spiflash_info_t *flash_info)
{
    sunxi_hal_spinor_info *spinor_info;

    // CSI_PARAM_CHK(spiflash, CSI_ERROR);
    CSI_PARAM_CHK(flash_info, CSI_ERROR);
    spinor_info = hal_spinor_get_info();
    if (spinor_info == NULL) {
        return CSI_ERROR;
    }
    // flash_info->flash_name  = param->flash_name;
    // flash_info->flash_id    = param->flash_id;
    flash_info->flash_size  = spinor_info->sector_count * spinor_info->sector_size;
    flash_info->sector_size = spinor_info->sector_size;
    flash_info->page_size   = spinor_info->page_size;
    flash_info->xip_addr = 0;
    return CSI_OK;
}

/**
  \brief       Read data from Flash
  \param[in]   spiflash   SPIFLASH handle to operate
  \param[in]   offset     Data address, offset address relative to zero
  \param[out]  data       Pointer to a buffer storing the data read from Flash
  \param[in]   size       Number of data items to read
  \return      If receive successful, this function shall return the num of data witch is received successful
               otherwise, the function shall return Error code
*/
int32_t csi_spiflash_read(csi_spiflash_t *spiflash, uint32_t offset, void *data, uint32_t size)
{
    // CSI_PARAM_CHK(spiflash, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    if (hal_spinor_read_data(offset, data, size) != 0) {
        return CSI_ERROR;
    }
    return size;
}

/**
  \brief       Program data to Flash
  \param[in]   spiflash  SPIFLASH handle to operate
  \param[in]   offset    Data address, offset address relative to zero
  \param[in]   data      Pointer to a buffer containing the data to be programmed to Flash.
  \param[in]   size      Number of data items to program
  \return      If program successful, this function shall return the num of data witch is program successful,
               otherwise, the function shall return Error code
*/
int32_t csi_spiflash_program(csi_spiflash_t *spiflash, uint32_t offset, const void *data, uint32_t size)
{
    // CSI_PARAM_CHK(spiflash, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    LOG_D("offset: 0x%x, size: 0x%x\n", offset, size);
    if (hal_spinor_program_data(offset, data, size) != 0) {
        LOG_E("spinor write error.\n");
        return CSI_ERROR;
    }
    return size;
}

/**
  \brief       Erase Flash Sector
  \param[in]   spiflash    SPIFLASH handle to operate
  \param[in]   offset    Data address, offset address relative to zero
  \param[in]   size      Length to be erased
  \return      Error code
*/
csi_error_t csi_spiflash_erase(csi_spiflash_t *spiflash, uint32_t offset, uint32_t size)
{
    sunxi_hal_spinor_info *spinor_info;
    // CSI_PARAM_CHK(spiflash, CSI_ERROR);
    LOG_D("offset: 0x%x, size: 0x%x\n", offset, size);
    spinor_info = hal_spinor_get_info();
    CSI_PARAM_CHK(spinor_info, CSI_ERROR);
    if (size % spinor_info->sector_size) {
        LOG_E("spinor erase, size not sector align.\n");
        return CSI_ERROR;
    }
    if (hal_spinor_erase_sector(offset, size) != 0) {
        LOG_E("spinor erase error.\n");
        return CSI_ERROR;
    }
    return CSI_OK;
}

/**
  \brief       Read flash status register
  \param[in]   spiflash  SPIFLASH handle to operate
  \param[in]   cmd_code  Cmd code
  \param[out]  data      Data buf to save flash status register
  \param[in]   size      Register length in byte
  \return      Error code
*/
csi_error_t csi_spiflash_read_reg(csi_spiflash_t *spiflash, uint8_t cmd_code, uint8_t *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Write status register
  \param[in]   spiflash  SPIFLASH handle to operate
  \param[in]   cmd       Cmd code
  \param[out]  data      Data buf to save flash status register
  \param[in]   size      Register length in byte
  \return      Error code
*/
csi_error_t csi_spiflash_write_reg(csi_spiflash_t *spiflash, uint8_t cmd_code, uint8_t *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Enable spiflash write protection
  \param[in]   spiflash    SPIFLASH handle to operate
  \param[in]   offset      Protect flash offset,offset need protect block size aligned
  \param[in]   size        Lock size(byte)
  \return      Error code
*/
csi_error_t csi_spiflash_lock(csi_spiflash_t *spiflash, uint32_t offset, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Enable spiflash write protection
  \param[in]   spiflash    SPIFLASH handle to operate
  \param[in]   offset      Protect flash offset,offset need protect block size aligned
  \param[in]   size        Unlock size(byte)
  \return      Error code
*/
csi_error_t csi_spiflash_unlock(csi_spiflash_t *spiflash, uint32_t offset, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       check flash is locked(write protect)
  \param[in]   spiflash    SPIFLASH handle to operate
  \param[in]   offset      Protect flash offset,offset need protect block size aligned
  \param[in]   size        Locked size(byte)
  \return      0:unlocked if query region overlay with locked region 1: locked if query reigon is fully in locked region
*/
int csi_spiflash_is_locked(csi_spiflash_t *spiflash, uint32_t offset, uint32_t size)
{
    return 0;
}

/**
  \brief       Set QSPI frequence
  \param[in]   spiflash SPIFLASH handle to operate
  \param[in]   hz       SPIFLASH frequence
  \return      The actual config frequency
*/
uint32_t csi_spiflash_frequence(csi_spiflash_t *spiflash, uint32_t hz)
{
    return 0;
}

/**
  \brief       Flash power down.
  \param[in]   spiflash SPIFLASH handle to operate.
  \return      error code
*/
csi_error_t csi_spiflash_release_power_down(csi_spiflash_t *spiflash)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Flash power release.
  \param[in]   spiflash SPIFLASH handle to operate.
  \return      none
*/
void csi_spiflash_power_down(csi_spiflash_t *spiflash)
{
    
}
