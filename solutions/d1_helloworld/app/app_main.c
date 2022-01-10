/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "board.h"
#include <aos/kernel.h>
#include <stdio.h>
#include <ulog/ulog.h>

#define TAG "app"

extern void  cxx_system_init(void);
extern void board_yoc_init(void);

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    LOGI(TAG, "app start........\n");
    while (1) {
        LOGI(TAG, "hello world");
        aos_msleep(3000);
    };
}