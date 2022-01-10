/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <aos/kernel.h>
#include <aos/hal/touch.h>
#include <ulog/ulog.h>
#include "d1_port.h"
#include "app_main.h"
#include <aos/cli.h>
#include "lwip/tcpip.h"
#include "dbus_demo.h"

#define TAG "main"
extern void cxx_system_init(void);
extern void touchscreen_event(touch_message_t touch_msg);
extern int falcon_entry(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    hal_touch_init(touchscreen_event);
    printf("haasui entry here!\r\n");
    printf("haasui build time: %s, %s\r\n", __DATE__, __TIME__);

    {
        extern void sys_init(void);
        sys_init();
        tcpip_init(NULL, NULL);

        extern int dbus_daemon_start();
        dbus_daemon_start();
        aos_msleep(50);

        dbus_demo_start();
        aos_msleep(50);
    }

    falcon_entry(0, NULL);

    for (;;) {
        aos_msleep(2000);
    };
}
