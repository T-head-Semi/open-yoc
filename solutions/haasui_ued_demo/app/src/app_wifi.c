#include <stdbool.h>
#include "board.h"
#include "app_main.h"
#include <yoc/partition.h>
#include <yoc/init.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>
#include <uservice/eventid.h>
#include <uservice/event.h>
#include <ntp.h>
#ifdef CONFIG_WIFI_XR829
#include <devices/xr829.h>
#else
#include <devices/rtl8723ds.h>
#endif


#define TAG "init"

netmgr_hdl_t app_netmgr_hdl;

void user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    switch (event_id) {
        case EVENT_NETMGR_GOT_IP: {
            LOGI(TAG, "Got IP");
            printf("Got IP, you can input ifconfig cmd in cli console to check the IP.\n");
            event_publish(EVENT_NTP_RETRY_TIMER, NULL);
        } break;
        case EVENT_NETMGR_NET_DISCON: {
            LOGI(TAG, "Net down");
            printf("Net down\n");
        } break;
        case EVENT_NTP_RETRY_TIMER: {
            LOGI(TAG, "NTP Start");
            printf("NTP Start\n");
            if (ntp_sync_time(NULL) == 0) {
                event_publish(EVENT_NET_NTP_SUCCESS, NULL);
            } else {
                event_publish_delay(EVENT_NTP_RETRY_TIMER, NULL, 6000);
            }
         } break;
         case EVENT_NET_NTP_SUCCESS: {
            LOGI(TAG, "NTP Success");
            printf("NTP Success\n");
         } break;
    }
    /*do exception process */
    app_exception_event(event_id);
}

void app_network_init()
{
    /* init wifi driver and network */
#ifdef CONFIG_WIFI_XR829
    wifi_xr829_register(NULL);
#else
    /* init wifi driver and network */
    rtl8723ds_gpio_pin pin = {
        .wl_en = WLAN_ENABLE_PIN,
        .power = WLAN_POWER_PIN,
    };
    wifi_rtl8723ds_register(&pin);
#endif


    app_netmgr_hdl = netmgr_dev_wifi_init();

    if (app_netmgr_hdl) {
        utask_t *task = utask_new("netmgr", 10 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
        netmgr_service_init(task);
        netmgr_start(app_netmgr_hdl);
    }

    event_subscribe(EVENT_NETMGR_GOT_IP, user_local_event_cb, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, user_local_event_cb, NULL);

    /* 使用系统事件的定时器 */
    event_subscribe(EVENT_NTP_RETRY_TIMER, user_local_event_cb, NULL);
    event_subscribe(EVENT_NET_NTP_SUCCESS, user_local_event_cb, NULL);
}
