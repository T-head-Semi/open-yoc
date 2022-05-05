/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifdef CONFIG_SUPPORT_YMODEM

#include <string.h>
#include <stdint.h>
//#include "uart_drv.h"

#include "ymodem_porting.h"
#include "ymodem.h"

#ifdef CONFIG_ENABLE_BOOT_LED
#include "boot_led.h"
#endif

#include <board_config.h>
#include <aos/kernel.h>
#include <ulog/ulog.h>

#include "gateway/mesh.h"
#include "app_main.h"
#include "ota_server.h"
#include "ota_process.h"
#include "dfu_port.h"
#include "aos/hal/uart.h"
#include "platform/wm_fwup.h"
#include "gateway/ota.h"
#include "provisioner_main.h"

#define TAG "YMODEM"

/*****************************************************
 * 按键检查，确认是否进入Ymodem状态
 ****************************************************/
#include <pinmux.h>
#include <drv/gpio.h>

#define FLASH_BLOCK_SIZE (64*1024)
#define FLASH_SECTOR1 4096
#define CONFIG_YMODEM_BUAD_HIGH 1000000
#define UART_PORTING_SYNC 0


#ifdef CONFIG_YMODEM_MAGIC_PIN
static gpio_pin_handle_t g_check_key_hdl;
#endif

static int g_yomode_image_idx = -1;
static gateway_ota_image_t ymodem_img_addr = NULL;
static uint32_t ymodem_img_size = 0;

static int g_ymodel_ota_for_dev_type;
static int g_ymodel_ota_for_dev_protocol;
static int g_ymodel_ota_for_dev_channel;

extern void wifi_network_deinit();
extern int wifi_network_inited();

extern void atserver_stop(void);
extern void atserver_resume(void);
extern void at_report_firmware_index(int16_t index);
extern int  get_version_str(uint32_t version, char *ver_str);
extern void at_report_fw_status(uint8_t index, uint8_t status);
extern char *ymodem_get_file_name();
static void ymodem_ota_dev_event_cb(ota_device_state_en event_id, void *param);
static void ymodem_ota_firmware_event_cb(ota_firmware_state_en event, void *data);
extern void at_report_ota_status(uint16_t index, uint8_t status, mac_t addr, char *version);

typedef struct {
    char name[64];
    uint16_t index;
    uint8_t  type;
    uint8_t  flag;//0 remove/ 1 used
} app_firmware_info;

#define  MAX_APP_FIRMWARE_INFO_NUM 10

app_firmware_info g_app_firmwre[MAX_APP_FIRMWARE_INFO_NUM];

static struct ota_server_cb ymodem_ota_cb = {
    .device_event_cb  = ymodem_ota_dev_event_cb,
};

int ymodem_check(void)
{
#ifdef CONFIG_YMODEM_MAGIC_PIN
    bool mute_key_val;

    drv_pinmux_config(CONFIG_YMODEM_MAGIC_PIN, PIN_FUNC_GPIO);
    g_check_key_hdl = csi_gpio_pin_initialize(CONFIG_YMODEM_MAGIC_PIN, NULL);
    csi_gpio_pin_config_mode(g_check_key_hdl, GPIO_MODE_PULLNONE);
    csi_gpio_pin_config_direction(g_check_key_hdl, GPIO_DIRECTION_INPUT);

    extern void mdelay(uint32_t ms);
    mdelay(500);

    csi_gpio_pin_read(g_check_key_hdl, &mute_key_val);

    if (mute_key_val == 0) {
        return CONFIG_YMODEM_BUAD_HIGH;
    }

    return -1;
#else
    return CONFIG_YMODEM_BUAD_HIGH;
#endif
}

/*****************************************************
 * 串口收发移植接口
 ****************************************************/
#if UART_PORTING_SYNC
#include <drv/usart.h>
static usart_handle_t g_uart_handle;

int ymodem_init_porting(int baud_rate)
{
    /* init the console */
    g_uart_handle = csi_usart_initialize(YMODEM_UART_IDX, NULL);
    /* config the UART */
    return csi_usart_config(g_uart_handle, baud_rate, USART_MODE_ASYNCHRONOUS, USART_PARITY_NONE,
                            USART_STOP_BITS_1, USART_DATA_BITS_8);
}
int ymoedm_send_char(unsigned char ch)
{
    int ret;
    ret = csi_usart_putchar(g_uart_handle, ch);
    return ret;
}

int ymodem_rcv(unsigned char *data, int size)
{
    int ret = -1;
    ret     = csi_usart_receive_query(g_uart_handle, data, size);
    return ret;
}
#endif

/* ymodem porting */
#if UART_PORTING_SYNC == 0
int ymodem_init_porting(int baud_rate)
{
    ota_server_cb_register(&ymodem_ota_cb);
    atserver_stop();
    return 0;
}

int ymoedm_send_char(unsigned char ch)
{
    uart_dev_t      uart_dev;
    uart_dev.port = YMODEM_UART_IDX;
    hal_uart_send(&uart_dev, (void *)&ch, 1, AOS_WAIT_FOREVER);
    return 0;
}

int ymodem_rcv(unsigned char *data, int size)
{
    uart_dev_t      uart_dev;
    uart_dev.port = YMODEM_UART_IDX;
    uint32_t recv_len = 0;
    hal_uart_recv_II(&uart_dev, (void *)data, size, &recv_len, 100);

    return recv_len;
}
#endif

int ymodem_uninit_porting(void)
{
    atserver_resume();

#ifdef CONFIG_YMODEM_MAGIC_PIN
    return csi_gpio_pin_uninitialize(g_check_key_hdl);
#else
    return 0;
#endif

}

/*****************************************************
 * Flash写接口
 ****************************************************/

int ymodem_get_write_addr(uint8_t *data, uint32_t *flash_addr)
{
    *flash_addr = (uint32_t)ymodem_img_addr;
    return 0;
}

int ymodem_flash_read(uint32_t address, uint8_t *buf, uint32_t b_len)
{
    memcpy((void *)address, buf, b_len);
    return 0;
}

/* 判断是否需要擦除 */
static uint32_t g_erase_precheck_buf[FLASH_SECTOR1 / sizeof(uint32_t)];
static int sector_is_erased(uint32_t address, int sector_count)
{
    return 1;
}

#if READ_COMPARE
static int sector_is_seem(uint32_t address, uint8_t *buf, uint32_t b_len)
{
    return 0;
}
#endif

int ymodem_flash_write(uint32_t address, uint8_t *buf, uint32_t b_len)
{
    memcpy((void *)address, buf, b_len);
    ymodem_img_size = (address - (uint32_t)ymodem_img_addr) + b_len;
    return 0;
}

int ymodem_flash_erase(uint32_t address, uint32_t b_len)
{
    memset((void *)address, 0, b_len);
    return 0;
}

static aos_task_t ymodem_task;

int app_firmware_add(char *name, uint16_t index, uint8_t type)
{
    if (!name) {
        return -1;
    }

    for (int i = 0; i < MAX_APP_FIRMWARE_INFO_NUM; i++) {
        if (g_app_firmwre[i].flag == 0) {
            uint8_t name_len = (sizeof(g_app_firmwre[0].name) < strlen(name)) ? sizeof(g_app_firmwre[0].name) : strlen(name);
            memcpy(g_app_firmwre[i].name, name, name_len);
            g_app_firmwre[i].index = index;
            g_app_firmwre[i].type  = type;
            g_app_firmwre[i].flag = 1;
            return 0;
        }
    }

    LOGE(TAG, "App firmware full");
    return -1;
}

int app_firmware_rm(uint16_t index)
{
    for (int i = 0; i < MAX_APP_FIRMWARE_INFO_NUM; i++) {
        if (g_app_firmwre[i].index == index) {
            g_app_firmwre[i].flag = 0;
            return 0;
        }
    }

    LOGE(TAG, "App firmware not found");
    return -1;
}

int app_firmware_search(char *name)
{
    if (!name) {
        return -1;
    }

    for (int i = 0; i < MAX_APP_FIRMWARE_INFO_NUM; i++) {
        if (g_app_firmwre[i].flag == 1 && !memcmp(g_app_firmwre[i].name, name, strlen(name))) {
            return g_app_firmwre[i].index;
        }
    }

    LOGE(TAG, "App firmware not found");
    return -1;
}

int ymodem_add_ota_firmware(void)
{
    int ret = 0;

    if (g_ymodel_ota_for_dev_type == OTA_FOR_GATEWAY) {
        ret = gateway_ota_gateway_upgrade(ymodem_img_addr, ymodem_img_size, ymodem_ota_firmware_event_cb);
    } else if (g_ymodel_ota_for_dev_type == OTA_FOR_BLE_NODE) {
        ret = gateway_ota_node_upgrade_with_channel(ymodem_img_addr, ymodem_img_size, g_ymodel_ota_for_dev_protocol, g_ymodel_ota_for_dev_channel, NULL, 0, ymodem_ota_firmware_event_cb);
    }

    return ret;
}

void ymodem_handle_task(void *arg)
{
    int ret = MODE_WIFI_NORMAL;
    int baud_rate = ymodem_check();
    g_yomode_image_idx = -1;
    LOGI(TAG, "===> ymodem_task\r\n");

#if defined(CONFIG_GW_FOTA_EN) && CONFIG_GW_FOTA_EN && defined(CONFIG_GW_USE_YOC_BOOTLOADER) && (CONFIG_GW_USE_YOC_BOOTLOADER)
    extern void gateway_fota_stop(void);
    gateway_fota_stop();
#endif

#if defined(CONFIG_BT_MESH) && CONFIG_BT_MESH > 0
    gateway_mesh_suspend();
#endif

#if defined(CONFIG_GW_SMARTLIVING_SUPPORT) && CONFIG_GW_SMARTLIVING_SUPPORT
    smartliving_client_control(0);
#endif
#if defined(CONFIG_GW_NETWORK_SUPPORT) && CONFIG_GW_NETWORK_SUPPORT
    if (wifi_network_inited()) {
        wifi_network_deinit();
    }
#endif
    aos_msleep(1000);

    if (baud_rate > 0) {
        ymodem_init(baud_rate);

        if (ymodem_upgrade() == 0) {
            LOGI(TAG, "ymodem upgrade ok");
            g_yomode_image_idx = ymodem_add_ota_firmware();
        } else {
            if (ymodem_img_addr) {
                gataway_ota_image_free(ymodem_img_addr);
                ymodem_img_addr = NULL;
                g_yomode_image_idx = -1;
            }

            LOGW(TAG, "ymodem timeout");
        }
    }

    ymodem_uninit();

    at_report_firmware_index(g_yomode_image_idx);

    if (g_yomode_image_idx >= 0) {
        app_firmware_add(ymodem_get_file_name(), g_yomode_image_idx, g_ymodel_ota_for_dev_type);
    }

#if defined(CONFIG_BT_MESH) && CONFIG_BT_MESH > 0
    gateway_mesh_resume();
#endif

#if defined(CONFIG_GW_NETWORK_SUPPORT) && CONFIG_GW_NETWORK_SUPPORT
    ret = app_network_reinit();
#endif
#if defined(CONFIG_GW_FOTA_EN) && CONFIG_GW_FOTA_EN && defined(CONFIG_GW_USE_YOC_BOOTLOADER) && CONFIG_GW_USE_YOC_BOOTLOADER && defined(CONFIG_GW_NETWORK_SUPPORT) && CONFIG_GW_NETWORK_SUPPOR
    extern void gateway_fota_start(void);
    if (ret == MODE_WIFI_NORMAL) {
        gateway_fota_start();
    }
#endif
#if defined(CONFIG_GW_SMARTLIVING_SUPPORT) && CONFIG_GW_SMARTLIVING_SUPPORT
    if (ret == MODE_WIFI_NORMAL) {
        smartliving_client_control(1);
    }
#endif

    LOGI(TAG, "<=== ymodem_task\r\n");
    aos_task_exit(0);
}

int ymodem_ota_prepare(int ota_type,int protocol, int ota_channel)
{
    if ((ota_type == OTA_FOR_GATEWAY) || (ota_type == OTA_FOR_BLE_NODE)) {
        g_ymodel_ota_for_dev_type = ota_type;
		g_ymodel_ota_for_dev_protocol = protocol;
		g_ymodel_ota_for_dev_channel = ota_channel;
        LOGI(TAG, "ota type is %d", g_ymodel_ota_for_dev_type);
    } else {
        LOGE(TAG, "invalid ota type");
        return -1;
    }

    ymodem_img_addr = gataway_ota_image_alloc(NULL, CONFIG_OTA_GATEWAY_IMG_SIZE);

    if (ymodem_img_addr == NULL) {
        LOGE(TAG, "no place for gateway image!!!");
        return -1;
    }

    return 0;
}

void ymodem_mode_start(void)
{
    aos_task_new_ext(&ymodem_task, "ymodem_task", ymodem_handle_task, NULL, 1024 * 4, AOS_DEFAULT_APP_PRI + 1);
}

static void ymodem_ota_dev_event_cb(ota_device_state_en event_id, void *param)
{
    uint8_t version_str[20] = {0};
    int ret = 0;

    if (!ymodem_img_addr) {
        return;
    }

    uint8_t firmware_id = *(uint8_t *)param;

    if (firmware_id != g_yomode_image_idx) {
        return;
    }

    switch (event_id) {
    case OTA_STATE_ONGOING: {
        ota_state_ongoing *status = (ota_state_ongoing *)param;
        LOGI(TAG, "ota ready for firmware:%d dev: %02x:%02x:%02x:%02x:%02x:%02x type: %d by channel :%02x\r\n",   \
             status->firmware_index, status->dev_info.addr.val[5], status->dev_info.addr.val[4], status->dev_info.addr.val[3], status->dev_info.addr.val[2], \
             status->dev_info.addr.val[1], status->dev_info.addr.val[0], status->dev_info.addr.type, status->channel);
    }
    break;

    case OTA_STATE_SUCCESS: {
        ota_state_success *status = (ota_state_success *)param;
        LOGI(TAG, "ota success for firmware:%d dev: %02x:%02x:%02x:%02x:%02x:%02x type: %d old_version:0x%04x new_verison:0x%04x cost %d ms by channel :%02x\r\n\r\n", \
             status->firmware_index, status->dev_info.addr.val[5], status->dev_info.addr.val[4], status->dev_info.addr.val[3], status->dev_info.addr.val[2], status->dev_info.addr.val[1], status->dev_info.addr.val[0], \
             status->dev_info.addr.type, status->old_ver, status->new_ver, status->cost_time, status->channel);
        get_version_str(status->new_ver, version_str);
        at_report_ota_status(status->firmware_index, OTA_STATE_SUCCESS, status->dev_info.addr, version_str);
        struct bt_mesh_node_t *node = NULL;
		dev_addr_t addr = {0};
	    memcpy(&addr,&status->dev_info.addr,sizeof(mac_t));
        node = bt_mesh_provisioner_get_node_info_by_mac(addr);
        if(!node) {
            LOGE(TAG,"Get node by mac faild");
        } else {
            if (node->version != status->new_ver) {
                provisioner_node_version_set(bt_mesh_provisioner_get_node_id(node), status->new_ver);
            }
        }
    }
    break;

    case OTA_STATE_FAILD: {
        ota_state_fail *status = (ota_state_fail *)param;
        LOGE(TAG, "ota fail for firmware:%d dev: %02x:%02x:%02x:%02x:%02x:%02x type: %d reason:%02x by channel :%02x\r\n",  \
             status->firmware_index, status->dev_info.addr.val[5], status->dev_info.addr.val[4], status->dev_info.addr.val[3], status->dev_info.addr.val[2], status->dev_info.addr.val[1], status->dev_info.addr.val[0], \
             status->dev_info.addr.type, status->reason, status->channel);
        at_report_ota_status(status->firmware_index, OTA_STATE_FAILD, status->dev_info.addr, version_str);
        get_version_str(status->new_ver, version_str);
        struct bt_mesh_node_t *node = NULL;
        dev_addr_t addr = {0};
	    memcpy(&addr,&status->dev_info.addr,sizeof(mac_t));
        node = bt_mesh_provisioner_get_node_info_by_mac(addr);
        if (!node) {
            LOGE(TAG, "no node find");
            return;
        }
        if(status->reason == OTA_FAIL_INVAILD_VERSION && node->version != status->old_ver) {
            provisioner_node_version_set(bt_mesh_provisioner_get_node_id(node), status->old_ver);
        }
    }
    break;

    default:
        break;
    }
}

static void ymodem_ota_firmware_event_cb(ota_firmware_state_en event, void *data)
{
    ota_firmware_state_data *event_data = (ota_firmware_state_data *)data;

    if (g_yomode_image_idx != event_data->firmware_index) {
        return;
    }

    switch (event) {
    case FIRMWARE_STATE_IN_USE: {
        LOGD(TAG, "OTA firmware index :%02x in use", event_data->firmware_index);
        at_report_fw_status(event_data->firmware_index, FIRMWARE_STATE_IN_USE);
    }
    break;

    case FIRMWARE_STATE_END_USE: {
        LOGD(TAG, "OTA firmware index :%02x end use", event_data->firmware_index);
        at_report_fw_status(event_data->firmware_index, FIRMWARE_STATE_END_USE);
    }
    break;

    case FIRMWARE_STATE_REMOVED: {
        LOGD(TAG, "OTA firmware index :%02x removed", event_data->firmware_index);
        at_report_fw_status(event_data->firmware_index, FIRMWARE_STATE_REMOVED);
        app_firmware_rm(event_data->firmware_index);

        if (ymodem_img_addr) {
            gataway_ota_image_free(ymodem_img_addr);
            ymodem_img_addr = NULL;
            g_yomode_image_idx = -1;
        }
    }
    break;
    }
}

#endif