/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-02-08     Zhangyihong  the first version
 * 2018-04-03     XY           gt9xx for 1024 * 600
 * 2018-04-14     liu2guang    optimize int and rst to pin framework
 * 2017-08-08     XY           imxrt1052
 * 2018-10-29     XY
 */
#include <string.h>
#include <drv/common.h>
#include <drv/gpio_pin.h>
#include <drv/touchscreen.h>
#include "soc.h"
#include "touch_ll.h"
#ifndef CONFIG_KERNEL_NONE
#ifdef AOS_COMP_DEBUG
#include <debug/dbg.h>
#else
#define printk printf
#endif /*AOS_COMP_DEBUG*/
#else
#define printk printf
#endif

#define TP_INT_PIN PG14
#define TP_RST_PIN PG15

#ifndef TP_INT_PIN
#error "Please config touch panel INT pin."
#endif

#ifndef IIC_RETRY_NUM
#define IIC_RETRY_NUM 2
#endif

// #define GT9xx_TS_ADDR               (0x5D)

#define gt9xx_READ_XY_REG           (0x814E)    /* 坐标寄存器       */
#define gt9xx_CLEARBUF_REG          (0x814E)    /* 清除坐标寄存器   */
#define gt9xx_CONFIG_REG            (0x8047)    /* 配置参数寄存器   */
#define gt9xx_COMMAND_REG           (0x8040)    /* 实时命令         */
#define gt9xx_PRODUCT_ID_REG        (0x8140)    /* 产品ID           */
#define gt9xx_VENDOR_ID_REG         (0x814A)    /* 当前模组选项信息 */
#define gt9xx_CONFIG_VERSION_REG    (0x8047)    /* 配置文件版本号   */
#define gt9xx_CONFIG_CHECKSUM_REG   (0x80FF)    /* 配置文件校验码   */
#define gt9xx_FIRMWARE_VERSION_REG  (0x8144)    /* 固件版本号       */

#if 1
#define TPDEBUG(fmt, ...)  do {printk(fmt"\r\n", ##__VA_ARGS__);} while (0)
#else
#define TPDEBUG(...)
#endif

static csi_gpio_pin_t rst_pin, int_pin;

#include <sunxi_hal_twi.h>
static int __touch_read(uint16_t port, uint16_t addr, void *cmd_buf, size_t cmd_len, void *data_buf, size_t data_len)
{
    hal_twi_control(port, I2C_SLAVE, &addr);
    hal_twi_mem_read(port, (unsigned long)cmd_buf, cmd_len, data_buf, data_len);
    return 0;
}

static int __touch_write(uint16_t port, uint16_t addr, void *data_buf, size_t data_len)
{
    twi_msg_t msg;
    msg.flags = 0;
    msg.addr = addr;
    msg.len = data_len;
    msg.buf = data_buf;
    hal_twi_control(port, I2C_RDWR, &msg);
    return 0;
}

static void gpio_pin_interrupt_handler(csi_gpio_pin_t *pin, void *arg)
{
    hal_sem_t hal_isr_sem = arg;
    csi_gpio_pin_irq_enable(&int_pin, false);
    // printk("%s, %d\r\n", __func__, __LINE__);
    hal_sem_post(hal_isr_sem);
}

static int __tp_irq_mode(hal_sem_t hal_isr_sem)
{
    int ret;

    /* Initialize pin periphrial */
    ret = csi_gpio_pin_init(&int_pin, TP_INT_PIN);
    if (ret) return -1;

    /* Attach callback */
    ret = csi_gpio_pin_attach_callback(&int_pin, gpio_pin_interrupt_handler, hal_isr_sem);
    if (ret) return -1;

    /* Set pull-up mode */
    ret = csi_gpio_pin_mode(&int_pin, GPIO_MODE_PULLUP);
    if (ret) return -1;

    /* Set input mode */
    ret = csi_gpio_pin_dir(&int_pin, GPIO_DIRECTION_INPUT);
    if (ret) return -1;

    /* Set falling-edge triger mode */
    ret = csi_gpio_pin_irq_mode(&int_pin, GPIO_IRQ_MODE_FALLING_EDGE);
    if (ret) return -1;

    /* Enable irq */
    ret = csi_gpio_pin_irq_enable(&int_pin, true);
    if (ret) return -1;
    return 0;
}

#ifdef TP_RST_PIN
static void gt9xx_hw_reset(uint8_t address)
{
    csi_gpio_pin_init(&rst_pin, TP_RST_PIN);
    csi_gpio_pin_mode(&rst_pin, GPIO_MODE_PULLNONE);
    csi_gpio_pin_dir(&rst_pin, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_init(&int_pin, TP_INT_PIN);
    csi_gpio_pin_mode(&int_pin, GPIO_MODE_PULLNONE);
    csi_gpio_pin_dir(&int_pin, GPIO_DIRECTION_OUTPUT);

    if (address == 0x5D)
    {
        csi_gpio_pin_write(&rst_pin, GPIO_PIN_LOW);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_LOW);
        mdelay(5);
        csi_gpio_pin_write(&rst_pin, GPIO_PIN_HIGH);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_LOW);
        mdelay(5);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_LOW);
        mdelay(5);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_HIGH);
    }
    else
    {
        csi_gpio_pin_write(&rst_pin, GPIO_PIN_LOW);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_HIGH);
        mdelay(3);
        csi_gpio_pin_write(&rst_pin, GPIO_PIN_HIGH);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_HIGH);
        mdelay(3);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_LOW);
        mdelay(3);
        csi_gpio_pin_write(&int_pin, GPIO_PIN_HIGH);
    }
}
#else
static void gt9xx_soft_reset(uint16_t slave_addr)
{
    uint8_t buf[3];

    buf[0] = (uint8_t)((gt9xx_COMMAND_REG >> 8) & 0xFF);
    buf[1] = (uint8_t)(gt9xx_COMMAND_REG & 0xFF);
    buf[2] = 0x02;
    __touch_write(slave_addr, buf, 3);
}
#endif

static int gt9xx_probe(uint16_t port, uint16_t slave_addr)
{
    uint8_t cmd[2];
    uint8_t buffer[5] = {0};

    TPDEBUG("[TP] start to probe![%d, 0x%x]\n", port, slave_addr);

#ifdef TP_RST_PIN
    gt9xx_hw_reset(slave_addr);
#else
    gt9xx_soft_reset(slave_addr);
#endif
    mdelay(5);

    cmd[0] = (uint8_t)((gt9xx_PRODUCT_ID_REG >> 8) & 0xFF);
    cmd[1] = (uint8_t)(gt9xx_PRODUCT_ID_REG & 0xFF);
    if (__touch_read(port, slave_addr, &cmd, 2, buffer, 4) != 0)
    {
        TPDEBUG("[TP] %s failed!\n", __func__);
        return -1;
    }

    buffer[4] = '\0';

    TPDEBUG("%#X %#X %#X %#X %#X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

    if(!strcmp((const char*)buffer, "911"))
    {
        TPDEBUG("[TP] Found chip gt911\n");
        return 0;
    }
    else if(!strcmp((const char*)buffer, "928"))
    {
        TPDEBUG("[TP] Found chip gt928\n");
        return 0;
    }
    else if(!strcmp((const char*)buffer, "9147"))
    {
        TPDEBUG("[TP] Found chip gt9147\n");
        return 0;
    }
    else if(!strcmp((const char*)buffer, "9157"))
    {
        TPDEBUG("[TP] Found chip gt9157\n");
        return 0;
    }
    else
    {
        TPDEBUG("[TP] Uknow chip gt9xx device: [%s]\n", buffer);
    }

    return -1;
}

static int gt9xx_init(touch_cls_t *touch_cls, touch_config_t *config)
{
    uint16_t slave_addr, port;
    uint8_t buf = 0;
    uint8_t cmd[2];

    TPDEBUG("[TP] GT9xx init\n");
    if (!(touch_cls && config)) {
        return -1;
    }
    memcpy(&touch_cls->config, config, sizeof(touch_config_t));
    slave_addr = touch_cls->config.slave_addr;
    port = touch_cls->config.iic_port;
    if (gt9xx_probe(port, slave_addr)) {
        return -1;
    }

    cmd[0] = (uint8_t)((gt9xx_CONFIG_VERSION_REG >> 8) & 0xFF);
    cmd[1] = (uint8_t)(gt9xx_CONFIG_VERSION_REG & 0xFF);
    __touch_read(port, slave_addr, &cmd, 2, &buf, 1);
    TPDEBUG("[TP] GT9xx Config version: 0x%02X\n", buf);

    cmd[0] = (uint8_t)((gt9xx_VENDOR_ID_REG >> 8) & 0xFF);
    cmd[1] = (uint8_t)(gt9xx_VENDOR_ID_REG & 0xFF);
    __touch_read(port, slave_addr, &cmd, 2, &buf, 1);
    TPDEBUG("[TP] GT9xx Sensor id: 0x%02X\n", buf);

    if (__tp_irq_mode(touch_cls->config.hal_isr_sem)) {
        TPDEBUG("[TP] GT9xx irq mode failed!\n");
        return -1;
    }
    return 0;
}

static void gt9xx_deinit(touch_cls_t *touch_cls)
{
    (void)touch_cls;
}

static int gt9xx_read_point(touch_cls_t *touch_cls, touch_message_t msg)
{
    uint16_t slave_addr, port;
    uint8_t cmd[2];
    uint8_t buf[8] = {0};
    static uint8_t s_tp_down = 0;

    if (!(touch_cls && msg)) {
        return -1;
    }
    slave_addr = touch_cls->config.slave_addr;
    port = touch_cls->config.iic_port;

    cmd[0] = (uint8_t)((gt9xx_READ_XY_REG >> 8) & 0xFF);
    cmd[1] = (uint8_t)(gt9xx_READ_XY_REG & 0xFF);
    __touch_read(port, slave_addr, &cmd, 2, buf, 8);

    if((buf[0] & 0x01) == 0)
    {
        if(s_tp_down)
        {
            s_tp_down = 0;
            msg->event = TOUCH_EVENT_UP;
        }
        else
        {
            msg->event = TOUCH_EVENT_NONE;
        }
    }
    else
    {
        msg->x = ((uint16_t)buf[3] << 8) | buf[2];
        msg->y = ((uint16_t)buf[5] << 8) | buf[4];

        if(s_tp_down)
        {
            msg->event = TOUCH_EVENT_MOVE;
        }
        else
        {
            msg->event = TOUCH_EVENT_DOWN;
            s_tp_down = 1;
        }
    }

    buf[0] = ((gt9xx_CLEARBUF_REG >> 8) & 0xFF);
    buf[1] = (gt9xx_CLEARBUF_REG & 0xFF);
    buf[2] = 0x00;
    __touch_write(port, slave_addr, buf, 3);
    csi_gpio_pin_irq_enable(&int_pin, true);

    return 0;
}

static touch_cls_t g_gt9xx_cls = {
    .init = gt9xx_init,
    .read_point = gt9xx_read_point,
    .deinit = gt9xx_deinit
};

void touch_gt9xx_register(touch_cls_t *touch_cls)
{
    if (touch_cls) {
        memcpy(touch_cls, &g_gt9xx_cls, sizeof(touch_cls_t));
    }
}