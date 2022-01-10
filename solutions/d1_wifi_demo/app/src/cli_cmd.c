/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <aos/cli.h>

extern void cli_reg_cmd_kvtool(void);
extern void cli_reg_cmd_iperf(void);
extern void cli_reg_cmd_ping(void);
extern void cli_reg_cmd_ifconfig(void);

void board_cli_init()
{
    aos_cli_init();
    cli_reg_cmd_kvtool();
    cli_reg_cmd_ping();
    cli_reg_cmd_ifconfig();
    cli_reg_cmd_iperf();
}
