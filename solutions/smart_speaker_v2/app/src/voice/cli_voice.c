/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <aos/cli.h>
#include <yoc/mic.h>

#define TAG "voice"

static int cli_voice_proc(int argc, char **argv)
{
    if (argc < 2) {
        return -1;
    }

    if (strcmp(argv[1], "wakeup")== 0) {
        int enable = atoi(argv[2]);
        aui_mic_control(MIC_CTRL_START_SESSION, enable, 1);
    }else {
        return -1;
    }
    return 0;
}

static void cmd_voice_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc >= 2) {
        if (cli_voice_proc(argc, argv) == 0) {
            return;
        }
    } else {
        printf("\tvoice wakeup 1\n");
    }
}

void cli_reg_cmd_voice(void)
{
    static const struct cli_command cmd_info = {"voice", "voice cmd test", cmd_voice_func};

    aos_cli_register_command(&cmd_info);
}
