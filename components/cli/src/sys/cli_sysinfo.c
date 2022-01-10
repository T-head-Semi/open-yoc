/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aos/cli.h>
#include <aos/kernel.h>
#include <yoc/sysinfo.h>

#define HELP_INFO \
    "Usage: sys <command>\n" \
    "\tos: show os version\n" \
    "\tapp: show app version\n" \
    "\tid: show device id\n" \
    "\treboot: reboot sys\n"

static void cmd_get_sysinfo_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc > 1) {
        if (0 == strcmp(argv[1], "os")) {
            printf("%s\n", aos_get_os_version());
            return;
        } else if (0 == strcmp(argv[1], "id")) {
            printf("%s\n", aos_get_device_id());
            return;
        } else if (0 == strcmp(argv[1], "app")) {
            printf("%s\n", aos_get_app_version());
            return;
        } else if (0 == strcmp(argv[1], "reboot")) {
            aos_reboot();
            return;
        } else if (0 == strcmp(argv[1], "sleep")) {
            if (argc == 3 && argv[2]) {
                aos_msleep(atoi(argv[2]) * 1000);
                return;
            }
        } else if (0 == strcmp(argv[1], "crash")) {
            // int *ptr = (int *)0x7f000001;
            // *ptr = 1;

            if (argc <= 2) {
                int a = 100;
                int b = 0;
                int c = a / b;
                printf("a/b=%d\r\n", c);
            } else {
                int type = atoi(argv[2]);
                switch(type) {
                    case 0: {
                        int *nullprt = NULL;
                        *nullprt = 1;
                        break;
                    }
                    case 1: {
                        typedef void (*func_ptr_t)();
                        func_ptr_t f = (func_ptr_t)0x12345678;
                        f();
                        break;
                    }
                    case 2: {
                        int a = 100;
                        int b = 0;
                        int c = a / b;
                        printf("a/b=%d\r\n", c);
                        break;
                    }
                    default: {
                        int a = 100;
                        int b = 0;
                        int c = a / b;
                        printf("a/b=%d\r\n", c);
                        break;
                    }
         
                }
            }
            return;
        }
    }

    printf(HELP_INFO);
}

void cli_reg_cmd_sysinfo(void)
{
    static const struct cli_command cmd_info = {
        "sys",
        "sys comand",
        cmd_get_sysinfo_func,
    };

    aos_cli_register_command(&cmd_info);
}
