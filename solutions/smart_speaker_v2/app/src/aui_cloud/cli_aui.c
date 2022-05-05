/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <stdlib.h>

#include <aos/cli.h>
#include <aos/kernel.h>
#include "pcm_test.h"
#include "aui_cloud/app_aui_cloud.h"

static void cmd_aui_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int len = 0;
    app_aui_cloud_start(0);

    while (len < pcm_len) {
        app_aui_cloud_push_audio(pcm + len, 640);
        len += 640;
        aos_msleep(15);
    }

    app_aui_cloud_stop(0);
}

void cli_reg_cmd_aui(void)
{
    static const struct cli_command cmd_info = {
        "aui",
        "aui tools",
        cmd_aui_func
    };

    aos_cli_register_command(&cmd_info);
}
