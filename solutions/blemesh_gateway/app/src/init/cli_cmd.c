/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <aos/debug.h>
#include <aos/cli.h>
#include <aos/kv.h>
#include <aos/kernel.h>
#include <vfs_cli.h>

extern void cli_reg_cmd_ping(void);
extern void cli_reg_cmd_ifconfig(void);
extern void cli_reg_cmd_kvtool(void);
extern void wifidrv_register_iwpriv_cmd(void);
extern void cli_reg_cmd_gateway_mesh(void);

void board_cli_init()
{
    aos_cli_init();

    cli_reg_cmd_kvtool();

    cli_reg_cmd_gateway_mesh();

#ifdef CONFIG_BTSOOP
    extern void cli_reg_cmd_btsnoop(void);
    cli_reg_cmd_btsnoop();
#endif

    extern void cli_reg_cmd_ping(void);
    cli_reg_cmd_ping();

    extern void cli_reg_cmd_ifconfig(void);
    cli_reg_cmd_ifconfig();

#ifdef CLI_CMD_DEBUG

    extern void cli_reg_cmd_occ(void);
    cli_reg_cmd_occ();

    extern void cli_reg_cmd_ble(void);
    cli_reg_cmd_ble();

    extern void cli_reg_cmd_blemesh(void);
    cli_reg_cmd_blemesh();

    // extern void cli_reg_cmd_bt(void);
    // cli_reg_cmd_bt();

    extern void cli_reg_cmd_iperf(void);
    cli_reg_cmd_iperf();

    extern void cli_reg_cmd_kp(void);
    cli_reg_cmd_kp();

    extern void cli_reg_cmd_http(void);
    cli_reg_cmd_http();

    /* fs cmd */
    cli_reg_cmd_ls();
    cli_reg_cmd_rm();
    cli_reg_cmd_cat();
    cli_reg_cmd_mkdir();
    cli_reg_cmd_mv();

    wifidrv_register_iwpriv_cmd();

    // cli_reg_cmd_duk();

    int ble_debug = 0;
    int ret;
    ret = aos_kv_getint("ble_debug", &ble_debug);
    if (ret < 0) {
        ble_debug = 0;
    }

    if (ble_debug) {
        LOGI("Init", "BLE Debug Mode, type `ble` or `blemesh` for support command");
        while(1)
        {
            aos_msleep(100000);
        }
    }
#endif
}
