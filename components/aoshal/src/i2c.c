/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <aos/kernel.h>
#include "aos/hal/i2c.h"
#include "drv/iic.h"



// #define    IIC_MODE_DMA
// #define    IIC_MODE_SYNC
// #define    IIC_MODE_INTR
typedef struct {
    aos_sem_t sem;
    aos_mutex_t mutex;
    csi_iic_t handle;
#ifdef IIC_MODE_DMA
    csi_dma_ch_t g_dma_ch_tx;
    csi_dma_ch_t g_dma_ch_rx;
#endif
} hal_i2c_priv_t;

static hal_i2c_priv_t iic_list[6];

static void i2c_cb(csi_iic_t *handle, csi_iic_event_t event)
{
    if ((event == IIC_EVENT_SEND_COMPLETE) || (event == IIC_EVENT_RECEIVE_COMPLETE)) {
        aos_sem_signal(&iic_list[handle->dev.idx].sem);
    } else {
        printf("i2c err event %d\n", event);
    }
}

int32_t hal_i2c_init(i2c_dev_t *i2c)
{
    csi_error_t ret;

    if (i2c == NULL) {
        return -1;
    }

    if (aos_mutex_new(&iic_list[i2c->port].mutex) != 0) {
        return -1;
    }

    ret = csi_iic_init(&iic_list[i2c->port].handle, i2c->port);

    if (ret != CSI_OK) {
        printf("csi_iic_init error\n");
        return -1;
    }

    if (i2c->config.mode == I2C_MODE_MASTER) {
        ret = csi_iic_mode(&iic_list[i2c->port].handle, IIC_MODE_MASTER);

        if (ret != CSI_OK) {
            printf("csi_iic_set_mode error\n");
            return -1;
        }
    }

    if (i2c->config.mode == I2C_MODE_SLAVE) {
        ret = csi_iic_mode(&iic_list[i2c->port].handle, IIC_MODE_SLAVE);

        if (ret != CSI_OK) {
            printf("csi_iic_set_mode error\n");
            return -1;
        }
    }

    if (i2c->config.address_width == I2C_HAL_ADDRESS_WIDTH_7BIT) {
        ret = csi_iic_addr_mode(&iic_list[i2c->port].handle, IIC_ADDRESS_7BIT);

        if (ret != CSI_OK) {
            printf("csi_iic_set_addr_mode error\n");
            return -1;
        }
    }

    if (i2c->config.address_width == I2C_HAL_ADDRESS_WIDTH_10BIT) {
        ret = csi_iic_addr_mode(&iic_list[i2c->port].handle, IIC_ADDRESS_10BIT);

        if (ret != CSI_OK) {
            printf("csi_iic_set_addr_mode error\n");
            return -1;
        }
    }

    switch (i2c->config.freq) {
        case I2C_BUS_BIT_RATES_100K:
            ret = csi_iic_speed(&iic_list[i2c->port].handle, IIC_BUS_SPEED_STANDARD);

            if (ret != CSI_OK) {
                printf("csi_iic_set_speed error\n");
                return -1;
            }

            break;

        case I2C_BUS_BIT_RATES_400K:
            ret = csi_iic_speed(&iic_list[i2c->port].handle, IIC_BUS_SPEED_FAST);

            if (ret != CSI_OK) {
                printf("csi_iic_set_speed error\n");
                return -1;
            }

            break;

        case I2C_BUS_BIT_RATES_3400K:
            ret = csi_iic_speed(&iic_list[i2c->port].handle, IIC_BUS_SPEED_HIGH);

            if (ret != CSI_OK) {
                printf("csi_iic_set_speed error\n");
                return -1;
            }

            break;

        default:
            break;
    }

    ret = csi_iic_own_addr(&iic_list[i2c->port].handle, i2c->config.dev_addr);

    if (ret != CSI_OK) {
        printf("csi_iic_set_speed error\n");
        return -1;
    }

#ifndef IIC_MODE_SYNC
    ret = csi_iic_attach_callback(&iic_list[i2c->port].handle, i2c_cb, NULL);

    if (ret != CSI_OK) {
        printf("csi_iic_attach_callback error\n");
        return -1;
    }

    if (aos_sem_new(&iic_list[i2c->port].sem, 0) != 0) {
        return -1;
    }

#endif

#ifdef IIC_MODE_DMA
    ret = csi_iic_link_dma(&iic_list[i2c->port].handle, &iic_list[i2c->port].g_dma_ch_tx, &iic_list[i2c->port].g_dma_ch_rx);

    if (ret != CSI_OK) {
        printf("csi_iic_link_dma fail \n");
        return -1;
    }

#endif

    return  ret;
}

int32_t hal_i2c_master_send(i2c_dev_t *i2c, uint16_t dev_addr, const uint8_t *data,
                            uint16_t size, uint32_t timeout)
{
    int32_t ret = 0;

    aos_mutex_lock(&iic_list[i2c->port].mutex, AOS_WAIT_FOREVER);

#ifdef IIC_MODE_SYNC
    int32_t num = 0;
    num = csi_iic_master_send(&iic_list[i2c->port].handle, dev_addr, data, size, timeout);

    if (num != size) {
        printf("csi_iic_master_send error\n");
        ret = -1;
        goto error;
    }

#endif

#ifndef IIC_MODE_SYNC
    ret = csi_iic_master_send_async(&iic_list[i2c->port].handle, dev_addr, data, size);

    if (ret < 0) {
        printf("csi_iic_master_send error\n");
        ret = -1;
        goto error;
    }

    ret = aos_sem_wait(&iic_list[i2c->port].sem, timeout);

    if (ret != 0) {
        ret = -1;
        printf("i2c timeout\n");
    }

#endif

error:
    aos_mutex_unlock(&iic_list[i2c->port].mutex);

    return ret;
}

int32_t hal_i2c_master_recv(i2c_dev_t *i2c, uint16_t dev_addr, uint8_t *data,
                            uint16_t size, uint32_t timeout)
{
    int32_t ret = 0;

    aos_mutex_lock(&iic_list[i2c->port].mutex, AOS_WAIT_FOREVER);
#ifdef IIC_MODE_SYNC
    int32_t num;
    num = csi_iic_master_receive(&iic_list[i2c->port].handle, dev_addr, data, size, timeout);

    if (num != size) {
        printf("csi_iic_master_receive error\n");
        ret = -1;
        goto error;
    }

#endif
#ifndef IIC_MODE_SYNC
    ret = csi_iic_master_receive_async(&iic_list[i2c->port].handle, dev_addr, data, size);

    if (ret < 0) {
        printf("csi_iic_master_receive error\n");
        goto error;
    }

    ret = aos_sem_wait(&iic_list[i2c->port].sem, timeout);

    if (ret != 0) {
        ret = -1;
        printf("i2c timeout\n");
    }

#endif
error:
    aos_mutex_unlock(&iic_list[i2c->port].mutex);

    return ret;
}

int32_t hal_i2c_slave_send(i2c_dev_t *i2c, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    int32_t ret = 0;

    aos_mutex_lock(&iic_list[i2c->port].mutex, AOS_WAIT_FOREVER);

#ifdef IIC_MODE_SYNC
    int32_t num;
    num = csi_iic_slave_send(&iic_list[i2c->port].handle, data, size, timeout);

    if (num != size) {
        printf("csi_iic_slave_send error\n");
        ret = -1;
        goto error;
    }

#endif
#ifndef IIC_MODE_SYNC
    ret = csi_iic_slave_send_async(&iic_list[i2c->port].handle, data, size);

    if (ret < 0) {
        printf("csi_iic_slave_send error\n");
        goto error;
    }

    ret = aos_sem_wait(&iic_list[i2c->port].sem, timeout);

    if (ret != 0) {
        ret = -1;
        printf("i2c timeout\n");
    }

#endif
error:
    aos_mutex_unlock(&iic_list[i2c->port].mutex);

    return ret;
}

int32_t hal_i2c_slave_recv(i2c_dev_t *i2c, uint8_t *data, uint16_t size, uint32_t timeout)
{
    int32_t ret = 0;

    aos_mutex_lock(&iic_list[i2c->port].mutex, AOS_WAIT_FOREVER);
#ifdef IIC_MODE_SYNC
    int32_t num;
    num = csi_iic_slave_receive(&iic_list[i2c->port].handle, data, size, timeout);

    if (num != size) {
        printf("csi_iic_slave_receive error\n");
        ret = -1;
        goto error;
    }

#endif
#ifndef IIC_MODE_SYNC
    ret = csi_iic_slave_receive_async(&iic_list[i2c->port].handle, data, size);

    if (ret < 0) {
        printf("csi_iic_slave_receive error\n");
        goto error;
    }

    ret = aos_sem_wait(&iic_list[i2c->port].sem, timeout);

    if (ret != 0) {
        ret = -1;
        printf("i2c timeout\n");
    }

#endif
error:
    aos_mutex_unlock(&iic_list[i2c->port].mutex);

    return ret;
}

int32_t hal_i2c_mem_write(i2c_dev_t *i2c, uint16_t dev_addr, uint16_t mem_addr,
                          uint16_t mem_addr_size, const uint8_t *data, uint16_t size,
                          uint32_t timeout)

{
    int32_t ret = 0;
    int32_t num = 0;
    aos_mutex_lock(&iic_list[i2c->port].mutex, AOS_WAIT_FOREVER);

    if (mem_addr_size == I2C_MEM_ADDR_SIZE_8BIT) {
        num = csi_iic_mem_send(&iic_list[i2c->port].handle, dev_addr, mem_addr, IIC_MEM_ADDR_SIZE_8BIT, data, size, timeout);
    }

    if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
        num = csi_iic_mem_send(&iic_list[i2c->port].handle, dev_addr, mem_addr, IIC_MEM_ADDR_SIZE_16BIT, data, size, timeout);
    }

    if (num != size) {
        printf("csi_iic_mem_write error\n");
        ret = -1;
        goto error;
    }

error:
    aos_mutex_unlock(&iic_list[i2c->port].mutex);
    return ret;
}

int32_t hal_i2c_mem_read(i2c_dev_t *i2c, uint16_t dev_addr, uint16_t mem_addr,
                         uint16_t mem_addr_size, uint8_t *data, uint16_t size,
                         uint32_t timeout)
{
    int32_t ret = 0;
    int32_t num = 0;
    aos_mutex_lock(&iic_list[i2c->port].mutex, AOS_WAIT_FOREVER);

    if (mem_addr_size == I2C_MEM_ADDR_SIZE_8BIT) {
        num = csi_iic_mem_receive(&iic_list[i2c->port].handle, dev_addr, mem_addr, IIC_MEM_ADDR_SIZE_8BIT, data, size, timeout);
    }

    if (mem_addr_size == I2C_MEM_ADDR_SIZE_16BIT) {
        num = csi_iic_mem_receive(&iic_list[i2c->port].handle, dev_addr, mem_addr, IIC_MEM_ADDR_SIZE_16BIT, data, size, timeout);
    }

    if (num != size) {
        printf("csi_iic_mem_read error\n");
        ret = -1;
        goto error;
    }

error:
    aos_mutex_unlock(&iic_list[i2c->port].mutex);
    return ret;
}

int32_t hal_i2c_finalize(i2c_dev_t *i2c)
{
    uint32_t ret = 0;

    if (i2c == NULL) {
        return -1;
    }

#ifdef IIC_MODE_DMA
    ret = csi_iic_link_dma(&iic_list[i2c->port].handle, NULL, NULL);

    if (ret != 0) {
        printf("csi_iic_link_dma fail \n");
    }

#endif
    csi_iic_detach_callback(&iic_list[i2c->port].handle);
    csi_iic_uninit(&iic_list[i2c->port].handle);

#ifndef IIC_MODE_SYNC
    aos_sem_free(&iic_list[i2c->port].sem);
#endif
    aos_mutex_free(&iic_list[i2c->port].mutex);

    return ret;
}
