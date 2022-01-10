#include <stdbool.h>
#include <drv/display.h>
#include <drv/g2d.h>
#include <aos/kv.h>
#include <vfs.h>
#include <vfs_cli.h>
#include <littlefs_vfs.h>
#include <debug/dbg.h>
#include <devices/xr829.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>
#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include <uservice/event.h>
#include <yoc/partition.h>
#include <yoc/init.h>
#include "board.h"
#include "app_main.h"

#define TAG "init"

netmgr_hdl_t app_netmgr_hdl;

void user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    if (event_id == EVENT_NETMGR_GOT_IP) {
        LOGI(TAG, "Got IP");
    } else if (event_id == EVENT_NETMGR_NET_DISCON) {
        LOGI(TAG, "Net down");
    } else {
        ;
    }
}

void network_init()
{
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

static void stduart_init(void)
{
    extern void console_init(int idx, uint32_t baud, uint16_t buf_size);
    console_init(CONSOLE_UART_IDX, 115200, 512);
}

static void fs_init(void)
{
    int ret;

    aos_vfs_init();
    ret = vfs_lfs_register("lfs");
    if (ret != 0) {
        LOGE(TAG, "littlefs register failed(%d)", ret);
        return;
    }
    LOGI(TAG, "filesystem init ok.");

    cli_reg_cmd_ls();
    cli_reg_cmd_rm();
    cli_reg_cmd_cat();
    cli_reg_cmd_mkdir();
    cli_reg_cmd_mv();
}

void board_yoc_init(void)
{
    board_init();
    stduart_init();
    board_cli_init();
    event_service_init(NULL);
    aos_debug_init();
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
        fs_init();
    }
    network_init();
}
