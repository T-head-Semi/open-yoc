/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/debug.h>
#include <yoc/netmgr_service.h>
#include <uservice/eventid.h>
#include "app_main.h"

#define TAG "AppExp"

#define MAX_NET_RESET_ERR_TIMES     100
#define NET_RESET_DELAY_TIME        3

static int net_reset_err_times =   0;

static void app_except_process(int errno, const char *file, int line, const char *func_name, void *caller)
{
    LOGE(TAG, "Except! errno is %s, function: %s at %s:%d, caller: 0x%x\n", strerror(errno), func_name, file, line, caller);

    while(1);
}

void app_exception_event(uint32_t event_id)
{
    switch(event_id) {
    case EVENT_NETMGR_NET_DISCON:
        LOGI(TAG, "EVENT_NETMGR_NET_DISCON");
        net_reset_err_times++;
        if (net_reset_err_times >= MAX_NET_RESET_ERR_TIMES) {
            LOGI(TAG, "Net Reset times %d, reboot", net_reset_err_times);
            //do reboot
            aos_reboot();
        } else {
            LOGI(TAG, "Net Reset after %d second", NET_RESET_DELAY_TIME);
            netmgr_reset(app_netmgr_hdl, NET_RESET_DELAY_TIME);
        }
        break;
    case EVENT_NETMGR_GOT_IP:
        net_reset_err_times = 0;
        break;
    }
}

void app_exception_init(void)
{
    /* Register system except process */
    aos_set_except_callback(app_except_process);

    /* Set utask default wdt timeout */
#ifdef CONFIG_DEBUG
    utask_set_softwdt_timeout(0);
#else
    utask_set_softwdt_timeout(60000);
#endif
}
