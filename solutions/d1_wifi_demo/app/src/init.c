#include <stdbool.h>
#include "board.h"
#include "app_main.h"
#include <yoc/partition.h>
#include <yoc/init.h>
#include <devices/xr829.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>
#include <uservice/eventid.h>
#include <uservice/event.h>
#include <vfs.h>
#include <vfs_cli.h>
#include <littlefs_vfs.h>

#define TAG "init"

netmgr_hdl_t app_netmgr_hdl;

void network_init()
{
    /* init wifi driver and network */
    wifi_xr829_register(NULL);
    app_netmgr_hdl = netmgr_dev_wifi_init();

    if (app_netmgr_hdl) {
        utask_t *task = utask_new("netmgr", 10 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
        netmgr_service_init(task);
        netmgr_start(app_netmgr_hdl);
    }
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
    event_service_init(NULL);

    console_init(CONSOLE_UART_IDX, 115200, 512);
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

    board_cli_init();
}
