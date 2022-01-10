/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_TCPIP)
#include <aos/cli.h>
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/stats.h"

#if LWIP_STATS && MEMP_STATS

extern struct stats_ lwip_stats;
extern const struct memp_desc* const memp_pools[];

static void cmd_lwip_mem_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int i;

    printf("LwIP memory heap info:\r\n"
           "avail: %8d, "
           "used : %8d, "
           "max  : %8d\r\n\r\n",
           lwip_stats.mem.avail, lwip_stats.mem.used, lwip_stats.mem.max);

    printf("LwIP has %d types of memory pools\r\n", MEMP_MAX);

    for (i = 0; i < MEMP_MAX; i++) {
        printf(
#if LWIP_STATS_DISPLAY
            "%s: %d\r\n"
#endif
            "avail: %8d, "
            "used : %8d, "
            "max  : %8d\r\n",
#if LWIP_STATS_DISPLAY
            lwip_stats.memp[i]->name, memp_pools[i]->size*lwip_stats.memp[i]->avail,
#endif
            lwip_stats.memp[i]->avail, lwip_stats.memp[i]->used, lwip_stats.memp[i]->max);
    }
}

void cli_reg_cmd_lwip_mem(void)
{
    static const struct cli_command cmd_info = {
        "lwipmem",
        "show lwip mem stat",
        cmd_lwip_mem_func
    };

    aos_cli_register_command(&cmd_info);
}
#endif
#endif

