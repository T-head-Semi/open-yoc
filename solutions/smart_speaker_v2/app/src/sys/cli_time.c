/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#if defined(AOS_COMP_CLI)
#include <aos/cli.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <drv/tick.h>
#include <ntp.h>

static void cmd_time_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;

    if (item_count == 2) {
        if (strcmp(argv[1], "us") == 0) {
            uint64_t us = csi_tick_get_us();
            printf("\ttick us: %ld\n", us);
        }
    } else if (item_count == 3) {
        if (strcmp(argv[1], "ntp") == 0) {
            ntp_sync_time(argv[2]);
        }
    } else {
        time_t t   = time(NULL);
        time_t lct = t + timezone * 3600 ;
        if (t >= 0) {
            printf("\tTZ(%02ld): %s %ld\n", timezone, ctime(&t), lct);
            printf("\t   UTC: %s %ld\n", asctime(gmtime(&t)), t);
        }
    }
}

void cli_reg_cmd_time(void)
{
    static const struct cli_command cmd_info = {"time", "time command.", cmd_time_func};

    aos_cli_register_command(&cmd_info);
}

#endif