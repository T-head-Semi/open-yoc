#include <devices/uart.h>
#include <yoc/atserver.h>
#include <ulog/ulog.h>
#include <uservice/eventid.h>
#include <uservice/uservice.h>
#include <aos/cli.h>

#include "app_main.h"

#define TAG "at_server_init"

extern int32_t at_cmd_event_on(void);

static void at_net_event_hdl(uint32_t event_id, const void *data, void *context)
{
    if (at_cmd_event_on()) {
        atserver_lock();

        if (event_id == EVENT_NETMGR_GOT_IP) {
            atserver_send("+EVENT=%s,%s\r\n", "NET", "LINK_UP");
        } else if (event_id == EVENT_NETMGR_NET_DISCON) {
            atserver_send("+EVENT=%s,%s\r\n", "NET", "LINK_DOWN");
        }

        atserver_unlock();
    }

    return;
}

void at_net_event_sub_init(void)
{
    event_subscribe(EVENT_NETMGR_GOT_IP, at_net_event_hdl, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, at_net_event_hdl, NULL);
}

void at_net_event_unsub_init(void)
{
    event_unsubscribe(EVENT_NETMGR_GOT_IP, at_net_event_hdl, NULL);
    event_unsubscribe(EVENT_NETMGR_NET_DISCON, at_net_event_hdl, NULL);
}

//收到串口at消息，然后根据对应at函数组包发送回去
void app_at_server_init(utask_t *task, const char *device_name)
{
    uart_config_t config;
    uart_config_default(&config);
    config.baud_rate = 1000000;

    if (task == NULL) {
        task = utask_new("at_svr", 4 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
    }

    if (task) {
        if (atserver_init(task, device_name, &config) == 0) {
            atserver_set_output_terminator("");
        } else {
            LOGE(TAG, "atserver_init fail");
        }
    }

    at_net_event_sub_init();

}
