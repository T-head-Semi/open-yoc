/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <aos/kernel.h>
#include <ulog/ulog.h>
#include "d1_port.h"
#include "app_main.h"
#include <aos/cli.h>

#define TAG "main"
extern void cxx_system_init(void);
extern void board_sound_init(void);
extern int cli_reg_cmd_xplayer(void);

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    board_sound_init();
    LOGI(TAG, "build time: %s, %s", __DATE__, __TIME__);

    cli_reg_cmd_xplayer();

    for (;;) {
        aos_msleep(2000);
    };
}

