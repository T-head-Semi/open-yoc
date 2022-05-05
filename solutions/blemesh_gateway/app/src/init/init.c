
#include <stdbool.h>
#include <aos/aos.h>
#include <yoc/yoc.h>
#include <yoc/partition.h>
#include <aos/kv.h>
#include <devices/rtl8723ds.h>
#include <devices/rtl8723ds.h>
#include <devices/uart.h>
#include <k_api.h>
#include <key_mgr.h>
#ifdef AOS_COMP_DEBUG
#include <debug/dbg.h>
#endif

#include "app_init.h"
#include <soc.h>
#include <vfs.h>
#include <vfs_cli.h>
#if defined (CONFIG_SAVE_JS_TO_RAM) && (CONFIG_SAVE_JS_TO_RAM == 1)
#else
#include <littlefs_vfs.h>
#endif

const char *TAG = "INIT";

#ifndef CONSOLE_UART_IDX
#define CONSOLE_UART_IDX 0
#endif


void board_yoc_init()
{
    board_init();

    console_init(CONSOLE_UART_IDX, 115200, 128);

    ulog_init();
    aos_set_log_level(AOS_LL_DEBUG);

    LOGI(TAG, "Build:%s,%s",__DATE__, __TIME__);
    /* load partition */
    int ret = partition_init();
    if (ret <= 0) {
        LOGE(TAG, "partition init failed");
    } else {
        LOGI(TAG, "find %d partitions", ret);
    }

    aos_kv_init("kv");

    km_init();

    extern int hci_h5_driver_init();
    hci_h5_driver_init();

    /* init wifi driver and network */
    rtl8723ds_gpio_pin pin = {
        .wl_en = PG12,
        .power = -1,
    };

    wifi_rtl8723ds_register(&pin);

    rtl8723ds_bt_config bt_config;

    bt_config.bt_dis_pin = PG18;
    bt_config.uart_id    = 1;

    bt_rtl8723ds_register(&bt_config);

    board_cli_init();

#ifdef AOS_COMP_DEBUG
    aos_debug_init();
#endif
#if defined (CONFIG_SAVE_JS_TO_RAM) && (CONFIG_SAVE_JS_TO_RAM == 1)
#else
    vfs_init();
    ret = vfs_lfs_register("lfs");
    if (ret != 0) {
        LOGE(TAG, "littlefs register failed(%d)", ret);
    }
#endif
}
