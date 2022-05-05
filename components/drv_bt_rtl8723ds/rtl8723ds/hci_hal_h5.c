/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "bt_hci_h5"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <aos/debug.h>

#include <devices/rtl8723ds_bt.h>
#include <devices/uart.h>
#include <devices/hal/hci_impl.h>
#include <devices/device.h>
#include <devices/driver.h>
#include <drv/gpio.h>
#ifdef CONFIG_CSI_V2
#include <soc.h>
#include <drv/gpio_pin.h>
#else
#include <pin_name.h>
#include <pinmux.h>
#endif
#include <devices/hci.h>

#include "bt_vendor_rtk.h"
#include "userial_vendor.h"

#define TAG "HCI"

// static aos_dev_t* uart_dev;
//static uart_config_t g_uart_config;
static void *g_priv;
static hci_event_cb_t  g_event;
static aos_event_t g_uart_event;
int g_uart_id;
int g_bt_dis_pin;


void hci_uart_entry(void *arg)
{
    uint32_t act_flag;

    while (1) {
        aos_event_get(&g_uart_event, 0x01, AOS_EVENT_OR_CLEAR, &act_flag, AOS_WAIT_FOREVER);
        if(g_event) {
            g_event(HCI_EVENT_READ, 0, g_priv);
        }
    }
}

static void uart_event(void *args)
{
    //if (event_id == USART_EVENT_READ || event_id == USART_OVERFLOW) {
        // printf("%s\n", __func__);
        aos_event_set(&g_uart_event, 0x1, AOS_EVENT_OR);
    //}
}

static int h5_hal_open(aos_dev_t *dev)
{
    tUSERIAL_CFG userial_init_cfg = {
        (USERIAL_DATABITS_8 | USERIAL_PARITY_EVEN | USERIAL_STOPBITS_1),
        USERIAL_BAUD_115200,
        USERIAL_HW_FLOW_CTRL_OFF,
        g_uart_id
    };
#ifdef CONFIG_CSI_V2
    csi_gpio_pin_t gpio;

    csi_gpio_pin_init(&gpio, g_bt_dis_pin);
    // aos_check(&gpio, EIO);
    csi_gpio_pin_dir(&gpio, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_write(&gpio, 0);
    aos_msleep(200);
    csi_gpio_pin_write(&gpio, 1);
#else
    gpio_pin_handle_t gpio;

    drv_pinmux_config(g_bt_dis_pin, PIN_FUNC_GPIO);
    gpio = csi_gpio_pin_initialize(g_bt_dis_pin, NULL);
    aos_check(gpio, EIO);
    csi_gpio_pin_config_mode(gpio, GPIO_MODE_PULLNONE);
    csi_gpio_pin_config_direction(gpio, GPIO_DIRECTION_OUTPUT);

    csi_gpio_pin_write(gpio, 0);
    aos_msleep(200);
    csi_gpio_pin_write(gpio, 1);
#endif
    aos_msleep(500);

    aos_check(!aos_event_new(&g_uart_event, 0), EIO);

    aos_task_t task_handle;
    aos_check(!aos_task_new_ext(&task_handle, "hci_uart", hci_uart_entry, NULL, 2048 * 2, 26), EIO);

    userial_vendor_open(&userial_init_cfg, uart_event);
    return 0;
}

static int h5_hal_close(aos_dev_t *dev)
{
    LOGD(TAG, "%s", __func__);

    userial_vendor_close();    

    return 0;
}

static int h5_send_data(aos_dev_t *dev, uint8_t *data, uint32_t size)
{
    return userial_vendor_send_data(data, size);
}

static int h5_recv_data(aos_dev_t *dev, uint8_t *data, uint32_t size)
{
    return userial_vendor_recv_data(data, size, 0);
}

static int h5_set_event(aos_dev_t *dev, hci_event_cb_t event, void *priv)
{
    g_event = event;
    g_priv = priv;

    return 0;
}

static int h5_start(aos_dev_t *dev, hci_driver_send_cmd_t send_cmd)
{
    hw_config_start(send_cmd);

    return 0;
}

static aos_dev_t *h5_hal_init(driver_t *drv, void *g_uart_config, int id)
{
    hci_driver_t *h5_dev = (hci_driver_t *)device_new(drv, sizeof(hci_driver_t), id);

    return (aos_dev_t *)h5_dev;
}

#define h5_hal_uninit device_free

static hci_driver_t h5_driver = {
    .drv = {
        .name   = "hci",
        .init   = h5_hal_init,
        .uninit = h5_hal_uninit,
        .lpm    = NULL,
        .open   = h5_hal_open,
        .close  = h5_hal_close,
    },
    .set_event = h5_set_event,
    .send      = h5_send_data,
    .recv      = h5_recv_data,
    .start     = h5_start,
};

/**
 * @brief  register bluetooth driver of rtl8723 
 * @param  [in] config
 * @return  
 */
void bt_rtl8723ds_register(rtl8723ds_bt_config *config)
{
    g_bt_dis_pin = config->bt_dis_pin;
    g_uart_id = config->uart_id;
    driver_register(&h5_driver.drv, NULL, 0);
}

