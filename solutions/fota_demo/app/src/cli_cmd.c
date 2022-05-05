#include <aos/cli.h>

extern void cli_reg_cmd_kvtool(void);
void cli_reg_cmd_ping(void);
void cli_reg_cmd_ifconfig(void);

void board_cli_init()
{
    aos_cli_init();

    cli_reg_cmd_ping();
    cli_reg_cmd_ifconfig();
    cli_reg_cmd_kvtool();
}
