/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_CLI) && ( defined(CONFIG_TCPIP) || defined(CONFIG_SAL))
#include <aos/cli.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void cmd_ntp_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;

    if (item_count >= 2) {
        ;
    } else {
        time_t t = time(NULL);
        time_t lct = t + timezone * 3600;
        if (t >= 0) {
            printf("\tTZ(%02ld): %s %ld\n", timezone, ctime(&t), lct);
            printf("\t   UTC: %s %ld\n", asctime(gmtime(&t)), t);
        }
    }
}

void cli_reg_cmd_ntp(void)
{
    static const struct cli_command cmd_info = {
        "date",
        "date command.",
        cmd_ntp_func
    };

    aos_cli_register_command(&cmd_info);
}

#endif /*defined(CONFIG_CLI) && ( defined(CONFIG_TCPIP)) */
