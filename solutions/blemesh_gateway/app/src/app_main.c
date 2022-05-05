/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include "aos/cli.h"
#include "app_main.h"
#include "app_sys.h"
#include "app_init.h"
// #include "w800_devops.h"
// #include "hci_hal_h4.h"
// #include <yoc/netmgr.h>
// #include <yoc/netmgr_service.h>
#include <yoc/atserver.h>
#include <uservice/eventid.h>
#include <uservice/uservice.h>
#include "aos/ble.h"
#include <aos/yloop.h>
#include "combo_net.h"
#include <board_config.h>

#include "linkkit_gateway/gateway_ut.h"
#include "gateway/mesh.h"
#include "ota_server.h"
#include "ota_process.h"
#include "dfu_port.h"
// #include "pin_name.h"
#include "app_voice.h"
#include "app_mesh.h"
#include "duktape_demo.h"
#include "gateway/occ_cmd.h"
#include <tsl_engine/device_all.h>
#include <tsl_engine/jse.h>
#include "gateway/ota.h"
#include "gateway/fota.h"
#ifdef CONFIG_BT_MESH_PROVISIONER
#include "inc/provisioner_main.h"
#endif


#define TAG "app"
extern int wifi_prov_method;

extern int tls_set_bt_mac_addr(uint8_t *mac);
extern int tls_get_bt_mac_addr(uint8_t *mac);
extern int tls_get_mac_addr(uint8_t *mac);
extern void cmd_show_dev_func(char *wbuf, int wbuf_len, int argc, char **argv);

static void cmd_prov_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (strcmp(argv[1], "0") == 0) {
        LOGD(TAG, "smart config");
        wifi_prov_method = WIFI_PROVISION_SL_SMARTCONFIG;
        aos_kv_setint("wprov_method", wifi_prov_method);
        app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
        aos_reboot();
    } else if (strcmp(argv[1], "1") == 0) {
        LOGD(TAG, "dev ap");
        wifi_prov_method = WIFI_PROVISION_SL_DEV_AP;
        aos_kv_setint("wprov_method", wifi_prov_method);
        app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
        aos_reboot();
    } else if (strcmp(argv[1], "2") == 0) {
        LOGD(TAG, "ble");
        wifi_prov_method = WIFI_PROVISION_SL_BLE;
        aos_kv_setint("wprov_method", wifi_prov_method);

        aos_kv_del("AUTH_AC_AS");
        aos_kv_del("AUTH_KEY_PAIRS");
        app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
        aos_reboot();
    } else if (strcmp(argv[1], "3") == 0) {
        LOGD(TAG, "softap");
        wifi_prov_method = WIFI_PROVISION_SOFTAP;
        aos_kv_setint("wprov_method", wifi_prov_method);
        app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
        aos_reboot();
    }
}

void cli_reg_cmd_wifi_prov(void)
{
    static const struct cli_command cmd_info = {
        "prov",
        "wifi prov commands",
        cmd_prov_func,
    };

    aos_cli_register_command(&cmd_info);
}

static void cmd_bt_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (strcmp(argv[1], "mac") == 0) {
        if (argc >= 3) {
            if (strlen(argv[2]) == 12) {
                uint8_t mac[6];
                char *p = argv[2];

                for (int i = 0; i < 6; ++i) {
                    uint8_t t = 0;

                    for (int j = 0; j < 2; ++j) {
                        t <<= 4;

                        if (*p >= '0' && *p <= '9') {
                            t += *p - '0';
                        } else if (*p >= 'a' && *p <= 'f') {
                            t += *p - 'a' + 10;
                        } else if (*p >= 'A' && *p <= 'F') {
                            t += *p - 'A' + 10;
                        } else {
                            printf("Usage: bt mac C01122334455\n");
                            return;
                        }

                        ++p;
                    }

                    mac[i] = t;
                }

                if (!tls_set_bt_mac_addr(mac)) {
                    printf("set bt mac successfully\n");
                } else {
                    printf("set bt mac failed\n");
                }
            } else {
                printf("Usage: bt mac C01122334455\n");
            }
        } else {
            uint8_t mac[6];

            if (!tls_get_bt_mac_addr(mac)) {
                printf("bt mac = %02X%02X%02X%02X%02X%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            } else {
                printf("bt mac not set\n");
            }
        }
    }
}

void cli_reg_cmd_bt(void)
{
    static const struct cli_command cmd_info = {
        "bt",
        "bt commands",
        cmd_bt_func,
    };

    aos_cli_register_command(&cmd_info);
}

static void cmd_mac_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    uint8_t mac_addr[6] = {0};

    tls_get_mac_addr(mac_addr);
    printf("mac addr %02x%02x%02x%02x%02x%02x\r\n", mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);
}


void cli_reg_cmd_mac(void)
{
    static const struct cli_command cmd_info = {
        "wifi_mac",
        "get wifi mac addr",
        cmd_mac_func,
    };

    aos_cli_register_command(&cmd_info);
}


static int chars2hex(const char *c, u8_t *x)
{
    if (*c >= '0' && *c <= '9') {
        *x = *c - '0';
    } else if (*c >= 'a' && *c <= 'f') {
        *x = *c - 'a' + 10;
    } else if (*c >= 'A' && *c <= 'F') {
        *x = *c - 'A' + 10;
    } else {
        return -EINVAL;
    }

    return 0;
}

static int str2_addr(const char *str, dev_addr_t *addr)
{
    int i, j;
    u8_t tmp;

    if (strlen(str) != 17) {
        return -EINVAL;
    }

    for (i = 5, j = 1; *str != '\0'; str++, j++) {
        if (!(j % 3) && (*str != ':')) {
            return -EINVAL;
        } else if (*str == ':') {
            i--;
            continue;
        }

        addr->val[i] = addr->val[i] << 4;

        if (chars2hex(str, &tmp) < 0) {
            return -EINVAL;
        }

        addr->val[i] |= tmp;
    }

    return 0;
}


static int str2bt_addr_le(const char *str, const char *type, dev_addr_t *addr)
{
    int err;

    err = str2_addr(str, addr);

    if (err < 0) {
        return err;
    }

    if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
        addr->type = DEV_ADDR_LE_PUBLIC;
    } else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
        addr->type = DEV_ADDR_LE_RANDOM;
    } else {
        return -EINVAL;
    }

    return 0;
}

extern int g_ota_for_dev_type;


static void cmd_add_ota_firmware_func(char *wbuf, int wbuf_len, int argc, char **argv)
{

    // device_info *devices = NULL;
    // uint8_t dev_num = 0;

    // int err = 0;

    if (argc < 3) {
        LOGE(TAG, "ERR arg");
        return;
    }

    firmware_info  firmware;
    firmware.address = (uint8_t *)strtoul(argv[1], NULL, 16);
    firmware.image_pos  = strtoul(argv[2], NULL, 16);


    if (firmware.image_pos != IMAGE_POS_FLASH && firmware.image_pos != IMAGE_POS_RAM) {
        LOGE(TAG, "invalid image position");
        return;
    }

    firmware.version = dfu_get_fota_file_app_version(firmware.address, firmware.image_pos);
    firmware.image_type = 0x00;
    firmware.size    = dfu_get_fota_file_size(firmware.address, firmware.image_pos);
    firmware.ota_chanel = strtoul(argv[3], NULL, 16);
    firmware.cb         =  NULL;
    if(argc > 3) {
        firmware.protocol   = strtoul(argv[4], NULL, 16);
    } else {
        firmware.protocol   = CONFIG_OTA_MODULE_DEFAULT_PROTOCAL;
    }

    if(firmware.protocol == MESH_OTA_PROTOCOL) {
        if(argc > 4) {
            firmware.multicast_addr = strtoul(argv[5], NULL, 16);
        } else {
            firmware.multicast_addr =  0x0000;
        }

        if(firmware.ota_chanel != OTA_BY_MESH_EXT_1M && firmware.ota_chanel != OTA_BY_MESH_EXT_2M && firmware.ota_chanel != OTA_BY_MESH_LEGACY) {
            LOGE(TAG,"Unpport ota channel for mesh ota protocol");
            return;
        }
    } else if(firmware.protocol == AIS_PROTOCOL) {
        if(firmware.ota_chanel == OTA_BY_MESH_EXT_1M || firmware.ota_chanel == OTA_BY_MESH_EXT_2M || firmware.ota_chanel == OTA_BY_MESH_LEGACY) {
            LOGE(TAG,"Unpport ota channel for ais ota protocol");
            return;
        }
    }


    if (firmware.ota_chanel == OTA_BY_GATT || firmware.ota_chanel == OTA_BY_MESH_EXT_1M || \
        firmware.ota_chanel == OTA_BY_MESH_EXT_2M || firmware.ota_chanel == OTA_BY_MESH_LEGACY) {
        g_ota_for_dev_type = 1;
    } else {
        g_ota_for_dev_type = 0;
    }

    if (firmware.size < 0) {
        LOGE(TAG, "Read fota image fail");
        return;
    } else {
        LOGD(TAG, "Fota image size %d", firmware.size);
    }

    int index  =  ota_server_upgrade_firmware_add(firmware);

    if (index < 0) {
        LOGE(TAG,"add ota firmware falid %d", index);
    } else {
        LOGD(TAG,"add ota firmware success index: %u image info: start address:0x%04x size: %d version: 0x%04x", index, firmware.address, firmware.size, firmware.version);
    }

}

static void cmd_rm_ota_firmware_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int err = 0;

    if (argc < 1) {
        LOGE(TAG, "ERR arg");
        return;
    }

    uint16_t index  = strtoul(argv[1], NULL, 16);

    err = ota_server_upgrade_firmware_rm(index);

    if (err) {
        LOGE(TAG, "rm ota firmware faild %d", err);
    } else {
        LOGD(TAG, "rm ota firmware success");
    }
}


static void cmd_add_ota_dev_func(char *wbuf, int wbuf_len, int argc, char **argv)
{

    device_info *devices = NULL;
    uint8_t dev_num = 0;
    // uint8_t ota_protocol  = 0;
    uint16_t index  = 0;
    ota_firmware *firmware = NULL;
    int err = 0;

    if (argc < 4) {
        LOGE(TAG, "ERR arg");
        return;
    }

    index = strtoul(argv[1], NULL, 16);

    dev_num = strtoul(argv[2], NULL, 16);

    argc -= 3;

    firmware = ota_server_upgrade_firmware_get(index);
    if(!firmware) {
        LOGE(TAG,"No firmware found for index %d",index);
        return;
    }

    uint8_t protocol =  firmware->firmware.info.protocol;


    if ((protocol == AIS_PROTOCOL  && argc < 2 * dev_num) ||  \
        (protocol == MESH_OTA_PROTOCOL && argc < dev_num)) {
        LOGE(TAG, "err dev info num");
        return;
    }

    devices = (device_info *)aos_zalloc(dev_num * sizeof(device_info));

    if (!devices) {
        LOGE(TAG, "malloc dev info fail");
        return;
    }

    int i = 0;

    if(protocol == MESH_OTA_PROTOCOL) {
        for(i = 0; i < dev_num; i++) {
            devices[i].unicast_addr = strtoul(argv[3 + i], NULL, 16);
#ifdef CONFIG_BT_MESH_PROVISIONER
            struct bt_mesh_node_t * node = bt_mesh_provisioner_get_node_info(devices[i].unicast_addr);
            if(!node) {
                LOGE(TAG, "Not found the node %04x in the provisioner", devices[i].unicast_addr);
                return;
            } else {
                memcpy(devices[i].addr.val, node->addr_val, 6);
                devices[i].addr.type = node->addr_type;
                devices[i].old_version = node->version;
            }
#endif
        }
    } else {
        for(i = 0; i < dev_num; i++) {
            err = str2bt_addr_le(argv[3 + 2 * i], argv[4 + 2 * i], (dev_addr_t *)&devices[i].addr);
            if (err) {
                LOGE(TAG, "Invalid peer address err %d\n", err);
                aos_free(devices);
                return;
            }
        }
    }

    err =   ota_server_upgrade_device_add(index, dev_num, devices);

    if (err) {
        LOGE(TAG, "Add ota dev faild %d", err);
    } else {
        LOGD(TAG, "Add ota dev success for image %d ", index);
    }

    aos_free(devices);

}

static void cmd_rm_ota_dev_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    device_info *devices = NULL;
    uint8_t dev_num = 0;
    uint16_t index  = 0;
    ota_firmware *firmware = NULL;
    int err = 0;

    if (argc < 4) {
        LOGE(TAG, "ERR arg");
        return;
    }

    index = strtoul(argv[1], NULL, 16);

    dev_num = strtoul(argv[2], NULL, 16);

    argc -= 2;

    firmware = ota_server_upgrade_firmware_get(index);
    if(!firmware) {
        LOGE(TAG,"No firmware found for index %d",index);
        return;
    }

    uint8_t protocol =  firmware->firmware.info.protocol;


    if ((protocol == AIS_PROTOCOL  && argc < 2 * dev_num) ||  \
        (protocol == MESH_OTA_PROTOCOL && argc < dev_num)) {
        LOGE(TAG, "err dev info num");
        return;
    }

    devices = (device_info *)aos_zalloc(dev_num * sizeof(device_info));
    if (!devices) {
        LOGE(TAG, "malloc dev info fail");
        return;
    }

    uint8_t i = 0;

    if(protocol == MESH_OTA_PROTOCOL) {
        for(i = 0; i < dev_num; i++) {
            devices[i].unicast_addr = strtoul(argv[3 + i], NULL, 16);
        }
    } else {
        for(i = 0; i < dev_num; i++) {
            err = str2bt_addr_le(argv[3 + 2 * i], argv[4 + 2 * i], (dev_addr_t *)&devices[i].addr);
            if (err) {
                LOGE(TAG, "Invalid peer address err %d\n", err);
                aos_free(devices);
                return;
            }
        }
    }

    err =   ota_server_upgrade_device_rm(index, dev_num, devices);

    if (err) {
        LOGE(TAG, "Del ota dev faild %d", err);
    } else {
        LOGD(TAG, "Del ota dev success for image %d ", index);
    }

    aos_free(devices);
}

static void cmd_ota_dev_start(char *wbuf, int wbuf_len, int argc, char **argv)
{

    // device_info *devices = NULL;
    // uint8_t dev_num = 0;
    // uint8_t ota_protocol  = 0;
    uint16_t index  = 0;
    ota_firmware *firmware = NULL;
    int err = 0;

    if (argc < 2) {
        LOGE(TAG, "ERR arg");
        return;
    }

    index = strtoul(argv[1], NULL, 16);

    firmware = ota_server_upgrade_firmware_get(index);
    if(!firmware) {
        LOGE(TAG,"No firmware found for index %d",index);
        return;
    }

    err =   ota_server_upgrade_start(index);
    if (err) {
        LOGE(TAG, "Ota dev start faild %d", err);
    } else {
        LOGD(TAG, "Ota dev start success for image %d ", index);
    }

}

#if defined(CONFIG_DEBUG) && CONFIG_DEBUG > 0
static void cmd_erase_misc(char *wbuf, int wbuf_len, int argc, char **argv)
{
    erase_dfu_flash(1);
}
#endif

static void cmd_play_test_voice(char *wbuf, int wbuf_len, int argc, char **argv)
{
    // int err = 0;

    if (argc < 1) {
        LOGE(TAG, "ERR arg");
        return;
    }

    uint16_t index  = strtoul(argv[1], NULL, 16);
    uint16_t voice  = strtoul(argv[2], NULL, 16);
    app_voice_play(index, voice);
}




void cli_reg_cmd_ota_firmware_add(void)
{
    static const struct cli_command cmd_info = {
        "addotafirmware",
        "addotafirmware address image_pos channel [protocol] [multiaddr]",
        cmd_add_ota_firmware_func,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ota_firmware_rm(void)
{
    static const struct cli_command cmd_info = {
        "rmotafirmware",
        "rmotafirmware index",
        cmd_rm_ota_firmware_func,
    };

    aos_cli_register_command(&cmd_info);
}


void cli_reg_cmd_ota_dev_add(void)
{

    static const struct cli_command cmd_info = {
        "addotadev",
        "addotadev index device_num [mac] [type] [unicast_addr]",
        cmd_add_ota_dev_func,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ota_dev_rm(void)
{
    static const struct cli_command cmd_info = {
        "rmotadev",
        "rmotadev index device_num [mac] [type] [unicast_addr]",
        cmd_rm_ota_dev_func,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ota_dev_start(void)
{
    static const struct cli_command cmd_info = {
        "otadevstart",
        "otadevstart index",
        cmd_ota_dev_start,
    };

    aos_cli_register_command(&cmd_info);
}

bool is_support_subdev_report_status(void)
{
    /*  1: is  support subdev report status
        0: not support subdev report status */
#if defined(CONFIG_SMARTLIVING_REPORT) && (CONFIG_SMARTLIVING_REPORT)
    return true;
#else
    return false;
#endif
}

bool is_support_occ_get_triples(void)
{
    /*  1: is  support occ get triples
        0: not support occ get triples */
#if defined(CONFIG_MESH_OCC_AUTH) && (CONFIG_MESH_OCC_AUTH)
    return true;
#else
    return false;
#endif
}

static void cmd_add_mesh_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    char mac[6] = {0x03, 0x00, 0x00, 0x4c, 0xe0, 0x00};
    char *cid = "ccf814f704400000a1a96f8ffb1cf103";
    uint8_t action = atoi(argv[1]);

    if (action == 1) {
        gateway_subdev_mesh_add((uint8_t*)mac, (uint8_t*)cid, 0);
    } else if (action == 2) {
        gateway_subdev_mesh_del((uint8_t*)mac);
    }
}


void cli_reg_add_mesh_test(void)
{
    static const struct cli_command cmd_info = {
        "add_mesh",
        "add mesh test",
        cmd_add_mesh_func,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_show_subdev(void)
{
    static const struct cli_command cmd_info = {
        "show_dev",
        "show the subdevs in the gateway",
        cmd_show_dev_func,
    };

    aos_cli_register_command(&cmd_info);
}

#if defined(CONFIG_DEBUG) && CONFIG_DEBUG > 0
void cli_reg_erase_misc(void)
{
    static const struct cli_command cmd_info = {
        "erasemisc",
        "erasemisc",
        cmd_erase_misc,
    };
    aos_cli_register_command(&cmd_info);
}
#endif


void cli_reg_cmd_play_test_voice(void)
{
    static const struct cli_command cmd_info = {
        "playvoice",
        "playvoice index volume",
        cmd_play_test_voice,
    };

    aos_cli_register_command(&cmd_info);
}


void cli_reg_gateway_cmd(void)
{
    cli_reg_add_mesh_test();
    cli_reg_show_subdev();
    cli_reg_cmd_ota_firmware_add();
    cli_reg_cmd_ota_firmware_rm();
    cli_reg_cmd_ota_dev_add();
    cli_reg_cmd_ota_dev_rm();
    cli_reg_cmd_ota_dev_start();
    cli_reg_cmd_play_test_voice();
#if defined(CONFIG_DEBUG) && CONFIG_DEBUG > 0
    cli_reg_erase_misc();
#endif
}

void app_rpt_fwver(char *version)
{
    atserver_send("+FWVER=%s\r\n", version);
}

// int app_factory_mode_check(void)
// {
    // gpio_pin_handle_t pin_hd;
    // bool pin_val = 0;

    // drv_pinmux_config(PA7, PIN_FUNC_GPIO);
    // pin_hd = csi_gpio_pin_initialize(PA7, NULL);

    // if (pin_hd == NULL) {
    //     LOGE(TAG, "init gpio fail");
    //     return -1;
    // }

    // csi_gpio_pin_config_direction(pin_hd, GPIO_DIRECTION_INPUT);
    // csi_gpio_pin_config_mode(pin_hd, GPIO_MODE_PULLDOWN);
    // csi_gpio_pin_read(pin_hd, &pin_val);

    // if (pin_val == 1) {
    //     LOGI(TAG, "enter factory mode");
    //     return 1;
    // } else {
    //     LOGI(TAG, "enter nomal mode");
    //     return 0;
    // }
// }

// void app_factory_mode(void)
// {
//     LOGD(TAG, "===>factory mode");

//     /* AT cmd related */
//     app_at_server_init(NULL, CONFIG_AT_UART_NAME);
//     app_at_cmd_init();

//     while (1) {
//         aos_msleep(10000);
//     }
// }


void app_restore_factory_setting()
{
    extern int ble_mesh_node_reset();
    ble_mesh_node_reset();//RST mesh

    extern int gateway_occ_triples_reset();
    gateway_occ_triples_reset();//RST triples

    extern int gateway_occ_url_reset();
    gateway_occ_url_reset();//RST occ url

    extern int gateway_subdev_ut_reset();
    gateway_subdev_ut_reset();//RST gateway ut

    extern int wifi_reset();
    wifi_reset();//RST wifi info

    extern int iotx_guider_clear_dynamic_url();
    iotx_guider_clear_dynamic_url();//Clear mqtt url

    aos_reboot();

}
int main()
{
    int err = 0;
    board_yoc_init();

#if defined(CONFIG_BT_MESH_EXT_ADV) && CONFIG_BT_MESH_EXT_ADV > 0
    LOGD(TAG, "Ble ext mesh gateway,Version %s\n", aos_get_app_version());
#else
    LOGD(TAG, "Ble mesh gateway,Version %s\n", aos_get_app_version());
#endif

#if defined(CONFIG_TSL_DEVICER_MESH) && CONFIG_TSL_DEVICER_MESH
    device_register_mesh();
    jse_init();
    jse_start();
    gateway_model_conv_init();
#endif

    aos_kv_setint("wifi_en", 1);

    event_service_init(NULL);

    // if (app_factory_mode_check() == 1) {
    //     app_factory_mode();
    // }

    app_sys_init();

    aos_loop_init();
#if defined(CONFIG_GW_SMARTLIVING_SUPPORT) && CONFIG_GW_SMARTLIVING_SUPPORT
    gateway_occ_get_gw_triples();
#endif
    gateway_ota_init();

    gateway_ut_init();

#if defined(CONFIG_GW_NETWORK_SUPPORT) && CONFIG_GW_NETWORK_SUPPORT
    app_network_init();
#endif

    // app_button_init();

    // app_at_server_init(NULL, "");

    // app_pwm_led_init();

    cli_reg_gateway_cmd();

    if (combo_is_breeze_start() == 0) {
        app_mesh_init();
    }

    err = gateway_fota_device_load();
    if(err) {
        LOGE(TAG,"App fota device load faild %d",err);
    }
    /* Registe AT Command after all service started */
    // app_at_cmd_init();

    // app_rpt_fwver(CONFIG_SDK_VERSION);
    aos_loop_run();

    return 0;
}
