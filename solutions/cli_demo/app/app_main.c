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
    LOGI(TAG, "Enter CLI shell \n");
    LOGI(TAG, "Try to type 'help' to get all commands description\n");
    while (1) {
        aos_msleep(3000);
    };
}