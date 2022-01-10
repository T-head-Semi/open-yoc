#include <stdbool.h>
#include "board.h"
#include "app_main.h"
// #include <devices/devicelist.h>
#include <yoc/partition.h>
#include <yoc/init.h>
// #include <devices/rtl8723ds.h>
#include <devices/xr829.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>
#include <uservice/eventid.h>
#include <uservice/event.h>


#define TAG "init"

// #define WLAN_ENABLE_PIN PB2
// // #define WLAN_ENABLE_PIN 0xFFFFFFFF
// #define WLAN_POWER_PIN 0xFFFFFFFF

netmgr_hdl_t app_netmgr_hdl;

void user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    if (event_id == EVENT_NETMGR_GOT_IP) {
        // g_net_gotip = 1;
        LOGI(TAG, "Got IP");
    } else if (event_id == EVENT_NETMGR_NET_DISCON) {
        // g_net_gotip = 0;
        LOGI(TAG, "Net down");
    } else {
        ;
    }
}

void network_init()
{
    /* init wifi driver and network */
    // rtl8723ds_gpio_pin pin = {
    //     .wl_en = WLAN_ENABLE_PIN,
    //     .power = WLAN_POWER_PIN,
    // };
    wifi_xr829_register(NULL);


    app_netmgr_hdl = netmgr_dev_wifi_init();

    if (app_netmgr_hdl) {
        utask_t *task = utask_new("netmgr", 10 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
        netmgr_service_init(task);
        netmgr_start(app_netmgr_hdl);
    }

    event_subscribe(EVENT_NETMGR_GOT_IP, user_local_event_cb, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, user_local_event_cb, NULL);
}

