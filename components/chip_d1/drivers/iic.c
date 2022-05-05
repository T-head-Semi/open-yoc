/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     iic.c
 * @brief
 * @version
 * @date     2020-07-06
 ******************************************************************************/

#include <string.h>
#include <sunxi_hal_twi.h>
#include <soc.h>
#include <drv/iic.h>

/**
  \brief       Init iic ctrl block
               Initializes the resources needed for the iic instance
  \param[in]   iic    Handle of iic instance
  \param[in]   idx    Index of instance
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_init(csi_iic_t *iic, uint32_t idx)
{
    CSI_PARAM_CHK(iic, CSI_ERROR);

    twi_port_t port = (twi_port_t)idx;
    iic->dev.idx = idx;
    if (port >= TWI_MASTER_MAX) {
        return CSI_ERROR;
    }
    twi_status_t ret = hal_twi_init(port);
    if (ret != TWI_STATUS_OK) {
        return CSI_ERROR;
    }

    return CSI_OK;
}

/**
  \brief       Uninit iic ctrl block
               Stops operation and releases the software resources used by the instance
  \param[in]   iic    Handle of iic instance
  \return      None
*/
void csi_iic_uninit(csi_iic_t *iic)
{
    CSI_PARAM_CHK_NORETVAL(iic);
    twi_port_t port = (twi_port_t)iic->dev.idx;
    twi_status_t ret = hal_twi_uninit(port);
    if (ret != TWI_STATUS_OK) {
    }
}

/**
  \brief       Config iic master or slave mode
  \param[in]   iic     Handle of iic instance
  \param[in]   mode    iic mode \ref csi_iic_mode_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_mode(csi_iic_t *iic, csi_iic_mode_t mode)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Config iic addr mode
  \param[in]   iic          Handle of iic instance
  \param[in]   addr_mode    iic addr mode \ref csi_iic_addr_mode_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_addr_mode(csi_iic_t *iic, csi_iic_addr_mode_t addr_mode)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Config iic speed
  \param[in]   iic      Handle of iic instance
  \param[in]   speed    iic speed mode \ref csi_iic_speed_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_speed(csi_iic_t *iic, csi_iic_speed_t speed)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Config iic own addr
  \param[in]   iic         Handle of iic instance
  \param[in]   own_addr    iic set own addr at slave mode
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_own_addr(csi_iic_t *iic, uint32_t own_addr)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start sending data as iic master
               This function is blocking
  \param[in]   iic       Handle of iic instance
  \param[in]   devaddr   Addrress of slave device
  \param[in]   data      Pointer to send data buffer
  \param[in]   size      Size of data items to send
  \param[in]   timout    Unit of time delay(ms)
  \return      The amount of real data sent or error code
*/
int32_t csi_iic_master_send(csi_iic_t *iic, uint32_t devaddr, const void *data, uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(iic, CSI_ERROR);
    twi_port_t port;
    twi_status_t ret;
    twi_msg_t msg;

    port = (twi_port_t)iic->dev.idx;
    msg.flags = 0;
    msg.addr = devaddr;
    msg.len = size;
    msg.buf = (uint8_t *)data;
    ret = hal_twi_control(port, I2C_RDWR, &msg);
    if (ret != TWI_STATUS_OK) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

/**
  \brief       Start receiving data as iic master
               This function is blocking
  \param[in]   iic        Handle to operate
  \param[in]   devaddr    iic addrress of slave device
  \param[out]  data       Pointer to buffer for data to receive from iic receiver
  \param[in]   size       Size of data items to receive
  \param[in]   timeout    Unit of time delay(ms)
  \return      The amount of real data received or error code
*/
int32_t csi_iic_master_receive(csi_iic_t *iic, uint32_t devaddr, void *data, uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(iic, CSI_ERROR);
    uint16_t addr;
    twi_port_t port;
    twi_status_t ret;

    port = (twi_port_t)iic->dev.idx;
    addr = (uint16_t)devaddr;
    ret = hal_twi_control(port, I2C_SLAVE, &addr);
    if (ret != TWI_STATUS_OK) {
        return CSI_ERROR;
    }
    ret = hal_twi_read(port, 0, data, size);
    if (ret != TWI_STATUS_OK) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

/**
  \brief       Start sending data as iic master
               This function is non-blocking,\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   iic     Handle to operate
  \param[in]   devaddr iic addrress of slave device
  \param[in]   data    Pointer to send data buffer
  \param[in]   size    Size of data items to send
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_master_send_async(csi_iic_t *iic, uint32_t devaddr, const void *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start receiving data as iic master.
               This function is non-blocking.\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   iic     Handle to operate
  \param[in]   devaddr iic addrress of slave device
  \param[out]  data    Pointer to buffer for data to receive from iic receiver
  \param[in]   size    Size of data items to receive
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_master_receive_async(csi_iic_t *iic, uint32_t devaddr, void *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start sending data as iic master
               This function is blocking
  \param[in]   iic             Handle of iic instance
  \param[in]   devaddr         Addrress of slave device
  \param[in]   memaddr         Internal addr of device
  \param[in]   memaddr_size    Internal addr mode of device
  \param[in]   data            Pointer to send data buffer
  \param[in]   size            Size of data items to send
  \param[in]   timout          Unit of time delay(ms)
  \return      The amount of real data sent or error code
*/
int32_t csi_iic_mem_send(csi_iic_t *iic, uint32_t devaddr, uint16_t memaddr, csi_iic_mem_addr_size_t memaddr_size, const void *data, uint32_t size, uint32_t timeout)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start receiving data as iic master
               This function is blocking
  \param[in]   iic             Handle to operate
  \param[in]   devaddr         iic addrress of slave device
  \param[in]   memaddr         Internal addr of device
  \param[in]   memaddr_mode    Internal addr mode of device
  \param[out]  data            Pointer to buffer for data to receive from eeprom device
  \param[in]   size            Size of data items to receive
  \param[in]   timeout         Unit of time delay(ms)
  \return      The amount of real data received or error code
*/
int32_t csi_iic_mem_receive(csi_iic_t *iic, uint32_t devaddr, uint16_t memaddr, csi_iic_mem_addr_size_t memaddr_size, void *data, uint32_t size, uint32_t timeout)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start sending data as iic slave
               This function is blocking
  \param[in]   iic        Handle to operate
  \param[in]   data       Pointer to buffer with data to send to iic master
  \param[in]   size       Size of data items to send
  \param[in]   timeout    Unit of time delay(ms)
  \return      The amount of real data sent or error code
*/
int32_t csi_iic_slave_send(csi_iic_t *iic, const void *data, uint32_t size, uint32_t timeout)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start receiving data as iic slave
               This function is blocking
  \param[in]   iic        Handle to operate
  \param[out]  data       Pointer to buffer for data to receive from iic master
  \param[in]   size       Size of data items to receive
  \param[in]   timeout    Unit of time delay(ms)
  \return      The amount of real data received or error code
*/
int32_t csi_iic_slave_receive(csi_iic_t *iic, void *data, uint32_t size, uint32_t timeout)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start sending data as iic slave
               This function is non-blocking,\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   iic     Handle to operate
  \param[in]   data    Pointer to buffer with data to send to iic master
  \param[in]   size    Size of data items to send
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_slave_send_async(csi_iic_t *iic, const void *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Start receiving data as iic slave
               This function is non-blocking,\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   handle  iic handle to operate
  \param[out]  data    Pointer to buffer for data to receive from iic master
  \param[in]   size    Size of data items to receive
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_slave_receive_async(csi_iic_t *iic, void *data, uint32_t size)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Attach callback to the iic
  \param[in]   iic    iic handle to operate
  \param[in]   cb     Event callback function \ref csi_iic_callback_t
  \param[in]   arg    User private param for event callback
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_attach_callback(csi_iic_t *iic, void *callback, void *arg)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Detach callback from the iic
  \param[in]   iic    iic handle to operate
  \return      None
*/
void csi_iic_detach_callback(csi_iic_t *iic)
{
}

/**
  \brief       Config iic stop to generate
  \param[in]   iic       iic handle to operate
  \param[in]   enable    Transfer operation is pending - stop condition will not be generated
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_xfer_pending(csi_iic_t *iic, bool enable)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Link DMA channel to iic device
  \param[in]   iic       Handle to operate
  \param[in]   tx_dma    The DMA channel handle for send, when it is NULL means to unlink the channel
  \param[in]   rx_dma    The DMA channel handle for receive, when it is NULL means to unlink the channel
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_link_dma(csi_iic_t *iic, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Get iic state
  \param[in]   iic      Handle to operate
  \param[out]  state    iic state \ref csi_state_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_get_state(csi_iic_t *iic, csi_state_t *state)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Enable iic power manage
  \param[in]   iic    iic handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_enable_pm(csi_iic_t *iic)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Disable iic power manage
  \param[in]   iic    iic handle to operate
  \return      None
*/
void csi_iic_disable_pm(csi_iic_t *iic)
{
}