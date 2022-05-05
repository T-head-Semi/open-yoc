/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <aos/cli.h>
#include "app_main.h"

extern void cli_reg_cmd_board_specific(void);
extern void cli_reg_cmd_kvtool(void);
extern void cli_reg_cmd_ping(void);
extern void cli_reg_cmd_iperf(void);
extern void cli_reg_cmd_ifconfig(void);
extern void cli_reg_cmd_aui(void);
extern void cli_reg_cmd_record(void);
extern void cli_reg_cmd_pcminput(void);
extern void cli_reg_cmd_time(void);
extern void cli_reg_cmd_voice(void);

void board_cli_init(void)
{
    cli_reg_cmd_board_specific();
    cli_reg_cmd_kvtool();
    cli_reg_cmd_ping();
    cli_reg_cmd_iperf();
    cli_reg_cmd_ifconfig();
    cli_reg_cmd_aui();
    cli_reg_cmd_record();
    cli_reg_cmd_pcminput();
    cli_reg_cmd_time();
    cli_reg_cmd_voice();
}
