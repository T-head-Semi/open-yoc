/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <aos/cli.h>
#include "app_main.h"

extern void cli_reg_cmd_kvtool(void);

void board_cli_init(void)
{
    aos_cli_init();
    cli_reg_cmd_kvtool();
}
