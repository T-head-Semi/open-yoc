/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <aos/cli.h>
#include <aos/kernel.h>
#include <aos/aos.h>

#define HELP_INFO "show memory info"

#ifdef CONFIG_TCPIP
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwipopts.h"

#if LWIP_STATS && MEMP_STATS

extern struct stats_ lwip_stats;
extern const struct memp_desc* const memp_pools[];
extern struct tcp_pcb *tcp_active_pcbs;

static uint32_t lwip_mem_func(void)
{
    int i;
    struct tcp_pcb *pcb;

    printf("LwIP memory heap info:\r\n"
           "avail: %8d, "
           "used:  %8d, "
           "max:   %8d\r\n\r\n",
           lwip_stats.mem.avail, lwip_stats.mem.used, lwip_stats.mem.max);

    printf("LwIP has %d types of memory pools\r\n", MEMP_MAX);

    for (i = 0; i < MEMP_MAX; i++) {
        printf(
#if LWIP_STATS_DISPLAY
            "%s: %d\r\n"
#endif
            "avail: %8d, "
            "used:  %8d, "
            "max:   %8d\r\n",
#if LWIP_STATS_DISPLAY
            lwip_stats.memp[i]->name, memp_pools[i]->size*lwip_stats.memp[i]->avail,
#endif
            lwip_stats.memp[i]->avail, lwip_stats.memp[i]->used, lwip_stats.memp[i]->max);
    }

    printf("\r\n");
    printf("SOCKET pcb status:  \r\n");

    if(tcp_active_pcbs == NULL) {
        printf("                   NO SOCKET LINK\r\n");
        return 0;
    }

    for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {

        if(pcb->state == 0) {
            printf("PCBaddr = %8p  "
                   "pcb_state = CLOSED \r\n", pcb);
        }

        if(pcb->state == 1) {
            printf("PCBaddr = %8p  "
                   "pcb_state = LISTEN \r\n", pcb);
        }

        if(pcb->state == 2) {
            printf("PCBaddr = %8p  "
                   "pcb_state = SYN_SENT \r\n", pcb);
        }

        if(pcb->state == 3) {
            printf("PCBaddr = %8p  "
                   "pcb_state = SYN_RCVD \r\n", pcb);
        }

        if(pcb->state == 4) {
            printf("PCBaddr = %8p  "
                   "pcb_state = ESTABLISHED \r\n", pcb);
        }

        if(pcb->state == 5) {
            printf("PCBaddr = %8p  "
                   "pcb_state = FIN_WAIT_1 \r\n", pcb);
        }

        if(pcb->state == 6) {
            printf("PCBaddr = %8p  "
                   "pcb_state = FIN_WAIT_2 \r\n", pcb);
        }

        if(pcb->state == 7) {
            printf("PCBaddr = %8p  "
                   "pcb_state = CLOSE_WAIT \r\n", pcb);
        }

        if(pcb->state == 8) {
            printf("PCBaddr = %8p  "
                   "pcb_state = CLOSING \r\n", pcb);
        }

        if(pcb->state == 9) {
            printf("PCBaddr = %8p  "
                   "pcb_state = LAST_ACK \r\n", pcb);
        }

        if(pcb->state == 10) {
            printf("PCBaddr = %8p  "
                   "pcb_state = LAST_ACK \r\n", pcb);
        }
    }

    printf("\r\n");

    return 0;
}
#endif
#endif//CONFIG_TCPIP

static uint32_t free_func(void)
{
    int total = 0,used = 0,mfree = 0,peak = 0;
    aos_get_mminfo(&total, &used, &mfree, &peak);
    printf("                   total      used      free      peak \r\n");
    printf("memory usage: %10d%10d%10d%10d\r\n\r\n",
           total, used, mfree, peak);

    return 0;
}

extern void aos_malloc_show(int mm);
void cmd_free_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 1) {
        free_func();
        return;
#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_MM)
    } else if ((argc == 2) && (0 == strcmp(argv[1], "mem"))) {
        aos_mm_dump();   /*use dumpsys mm_info; And freertos has no such debug api*/
        return;
    } else if ((argc >= 2) && (0 == strcmp(argv[1], "list"))) {
        free_func();
        //aos_malloc_show(argc == 3); //use dumpsys mm_info
        return;
#endif

#ifdef CONFIG_TCPIP
#if LWIP_STATS && MEMP_STATS
    } else if ((argc == 2) && (0 == strcmp(argv[1], "lwip"))) {
        lwip_mem_func();
#endif
#endif
    } else {
        printf("%s\r\n", HELP_INFO);
    }
}

void cli_reg_cmd_free(void)
{
    static const struct cli_command cmd_info = {
        "free",
        HELP_INFO,
        cmd_free_func
    };

    aos_cli_register_command(&cmd_info);
}
