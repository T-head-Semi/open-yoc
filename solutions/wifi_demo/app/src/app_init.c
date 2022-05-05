#include <stdbool.h>
#include "board.h"
#include "app_main.h"
#include <yoc/partition.h>
#include <yoc/init.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>
#include <uservice/eventid.h>
#include <uservice/event.h>
#ifdef CONFIG_WIFI_XR829
#include <devices/xr829.h>
#else
#include <devices/rtl8723ds.h>
#endif

#define TAG "init"

netmgr_hdl_t app_netmgr_hdl;

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
}

void board_yoc_init(void)
{
    board_init();
    console_init(CONSOLE_UART_IDX, 115200, 512);
    board_cli_init();
    printf("###YoC###[%s,%s]\n", __DATE__, __TIME__);
    printf("cpu clock is %dHz\n", soc_get_cpu_freq(0));
    event_service_init(NULL);
    ulog_init();
    aos_set_log_level(AOS_LL_DEBUG);
    int ret = partition_init();
    if (ret <= 0) {
        LOGE(TAG, "partition init failed");
    } else {
        LOGI(TAG, "find %d partitions", ret);
        if (aos_kv_init("kv")) {
            LOGE(TAG, "kv init failed.");
        }
    }
    app_network_init();
}
