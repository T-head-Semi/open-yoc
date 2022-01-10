#ifndef SUNXI_HAL_CMD_H
#define SUNXI_HAL_CMD_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef CONFIG_KERNEL_NONE
/* include me to supprot FINSH_FUNCTION_EXPORT_CMD */
#include <aos/cli.h>
#define FINSH_FUNCTION_EXPORT_CMD(name, cmd, desc) ALIOS_CLI_CMD_REGISTER(name, cmd, desc)
#else
#define FINSH_FUNCTION_EXPORT_CMD(name, cmd, desc)
#endif

#ifdef __cplusplus
}
#endif

#endif
