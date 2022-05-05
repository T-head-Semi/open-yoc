/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <aos/kernel.h>
#include <aos/hal/touch.h>
#include <ulog/ulog.h>
#include "board.h"
#include "d1_port.h"

extern void cxx_system_init(void);
extern void board_yoc_init(void);

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    ui_task_run();
    while (1) {
        aos_msleep(2000);
    };
}