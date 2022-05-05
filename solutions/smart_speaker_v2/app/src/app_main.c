/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <aos/kernel.h>
#include <aos/hal/touch.h>
#include <ulog/ulog.h>
#include <aos/cli.h>

#include "aui_cloud/app_aui_cloud.h"
#include "player/app_player.h"
#include "voice/app_audio.h"

#include "app_main.h"

#define TAG "main"

extern void cxx_system_init(void);
extern void network_init();
int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    printf("build time: %s, %s\r\n", __DATE__, __TIME__);

    app_player_init();

    network_init();

    app_mic_init();

    app_aui_nlp_init();

    while (1) {
        aos_msleep(2000);
    };
}
