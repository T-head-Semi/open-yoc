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
#include "avdbus/media_server_dbus.h"
#include "lwip/tcpip.h"

#define TAG "main"
extern void cxx_system_init(void);

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();

    {
        extern void sys_init(void);
        sys_init();
        tcpip_init(NULL, NULL);

        extern int dbus_daemon_start();
        dbus_daemon_start();
        aos_msleep(50);

        msvc_conf_t conf;

        media_server_config_init(&conf);
        media_server_init(&conf);
        media_server_start();

        extern int app_mic_init(void);
        app_mic_init();
    }

    ui_task_run();


    while (1) {
        aos_msleep(2000);
    };
}