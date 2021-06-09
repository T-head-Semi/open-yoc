/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <stdlib.h>
#include <string.h>

#include <aos/list.h>
#include <aos/debug.h>

#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include <yoc/sysinfo.h>
#include <board.h>

#include "app_main.h"


static void network_event(uint32_t event_id, const void *param, void *context)
{
    switch(event_id) {
    case EVENT_NETMGR_GOT_IP:
        break;
    case EVENT_NETMGR_NET_DISCON:

        break;
    }

    /*do exception process */
    app_exception_event(event_id);
}

char test[20];
int len = 20;
int main(void)
{
    board_yoc_init();

    /* Subscribe */
    event_subscribe(EVENT_NETMGR_GOT_IP, network_event, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, network_event, NULL);

    app_wifi_init();
}
