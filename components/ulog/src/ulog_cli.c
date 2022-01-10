/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#if defined (AOS_COMP_CLI) && (AOS_COMP_CLI)
#include <string.h>
#include <stdio.h>

#include <aos/cli.h>
#include <aos/kernel.h>
#include <ulog/ulog.h>

#define HELP_INFO \
    "Usage:\n\tset level: log level 0~7\n\t3:E 4:W 6:I 7:D"

static void cmd_log_ctrl_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int fmt_err = 1;
    if (argc >= 2) {
        if (0 == strcmp(argv[1], "level")) {
            if (argc == 3) {
                int level = LOG_DEBUG;
                switch(argv[2][0]) {
                    case 'E':
                    case 'e':
                        level = LOG_ERR;
                        break;
                    case 'W':
                    case 'w':
                        level = LOG_WARNING;
                        break;
                    case 'I':
                    case 'i':
                        level = LOG_INFO;
                        break;
                    case 'D':
                    case 'd':
                        level = LOG_DEBUG;
                        break;
                    default:
                        level = atoi(argv[2]);
                }
                if (level >=LOG_EMERG && level <= LOG_DEBUG ) {
                    printf("set log level %d\n", level);
                    aos_set_log_level(level);
                    fmt_err = 0;
                }
            }
        }
    }

    if (fmt_err) {
        printf(HELP_INFO);
    }
}

void cli_reg_cmd_log(void)
{
    static const struct cli_command cmd_info = {
        "log",
        "log contrtol",
        cmd_log_ctrl_func,
    };

    aos_cli_register_command(&cmd_info);
}

#endif // AOS_COMP_CLI