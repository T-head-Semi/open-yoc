/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <aos/cli.h>

extern void  cli_reg_cmd_board_specific(void);
extern void debug_cli_cmd_init(void);
void board_cli_init()
{
    aos_cli_init();
    cli_reg_cmd_board_specific();
    debug_cli_cmd_init();
}
