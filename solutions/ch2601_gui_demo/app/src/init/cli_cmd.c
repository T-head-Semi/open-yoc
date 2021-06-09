/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */


#include <aos/debug.h>
#include <aos/cli.h>

void board_cli_init()
{
    extern void cli_reg_cmd_help(void);
    cli_reg_cmd_help();

    extern void cli_reg_cmd_ps(void);
    cli_reg_cmd_ps();

    extern void cli_reg_cmd_free(void);
    cli_reg_cmd_free();

    extern void cli_reg_cmd_debug_func(void);
    cli_reg_cmd_debug_func();
}
