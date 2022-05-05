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

#define TAG "main"
extern void cxx_system_init(void);

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    ui_task_run();
    while (1) {
        aos_msleep(2000);
    };
}
