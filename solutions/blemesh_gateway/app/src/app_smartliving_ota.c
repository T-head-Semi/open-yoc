/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#include "stdio.h"
#include <aos/kernel.h>
#include <aos/debug.h>
#include <ulog/ulog.h>
#include <k_api.h>
// #include "platform/wm_fwup.h"
#include "app_main.h"
#include <ota_server.h>
#include "gateway/ota.h"
#include <dfu_port.h>
#include "smartliving/exports/iot_export_errno.h"
#include "smartliving/exports/iot_export_ota.h"
#include "linkkit_gateway/gateway_ut.h"
#include "provisioner_main.h"

#define TAG "SLOTA"



extern int dm_ota_get_ota_handle(void **handle);
extern void gateway_mesh_mac_convert(uint8_t *mac, uint8_t *mac_converted);

extern gw_subdev_info_t *gateway_subdev_info;
static int g_smartliving_image_idx = -1;
static int g_ota_from_smartliving = 0;
static char *smartliving_ota_image = NULL;
static uint32_t smartliving_ota_image_size = 0;

extern int g_ota_for_dev_type;
extern char *gateway_subdev_ota_map;
extern char ble_node_version[128];
int app_smartliving_fota_version_rpt(char *version);
static void smartliving_ota_dev_event_cb(ota_device_state_en event_id, void *param);
static void smartliving_ota_firmware_event_cb(ota_firmware_state_en event, void *data);

static struct ota_server_cb smartliving_occ_cb = {
    .device_event_cb  = smartliving_ota_dev_event_cb,
};

static int str2bt_addr(const char *str, uint8_t *addr_val)
{
    const char *p = str;

    if (strlen(str) != 12) {
        return -EINVAL;
    }

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
                return -1;
            }
            ++p;
        }
        addr_val[i] = t;
    }

    return 0;
}

void HAL_Firmware_Persistence_Start(uint32_t file_size)
{
    ota_server_cb_register(&smartliving_occ_cb);

    if (smartliving_ota_image == NULL) {
        smartliving_ota_image = aos_zalloc(CONFIG_OTA_GATEWAY_IMG_SIZE);

        if (smartliving_ota_image == NULL) {
            g_smartliving_image_idx = -1;
            LOGE(TAG, "no place for gateway image!!!");
        }
    }

#ifdef WIFI_PROVISION_ENABLED
    extern int awss_suc_notify_stop(void);
    awss_suc_notify_stop();
#endif
#ifdef DEV_BIND_ENABLED
    extern int awss_dev_bind_notify_stop(void);
    awss_dev_bind_notify_stop();
#endif
}

int app_smartliving_node_upgrade_check(uint8_t mac[6], uint32_t version)
{
    int ret  = 0;
    uint8_t ble_nodes_finished = 1;
    char mac_conv[6] = {0};
    char dn_mac[6] = {0};
    iotx_linkkit_dev_meta_info_t *p_subdev = NULL;
    // uint8_t ota_ongoing = 0;

    gateway_mesh_mac_convert(mac, (uint8_t*)mac_conv);

    for (int i = 1; i < MAX_DEVICES_META_NUM + 1; i++) {
        if (gateway_subdev_info->subdev_info[i].cloud_devid != GATEWAY_NODE_INVAL) {
            p_subdev = &(gateway_subdev_info->subdev_info[i].linkkit_dev_meta_info);
            gateway_subdev_dn2mac(p_subdev->device_name, dn_mac);
            if (memcmp(dn_mac, mac_conv, 6) == 0) {
                LOGI(TAG, "subdev[%d] ota finished", i);
                gateway_subdev_ota_map[i] = 0;
                // just for demo, rpt to cloud
                break;
            }
        }
    }

    for (int i = 1; i < MAX_DEVICES_META_NUM + 1; i++) {
        if (gateway_subdev_ota_map[i] == 1) {
            ble_nodes_finished = 0;
            break;
        }
    }

    if (ble_nodes_finished == 1) {
        if (g_ota_from_smartliving) { //remove the firmware directly when firmware from smartliving
            ret = ota_server_upgrade_firmware_rm(g_smartliving_image_idx);

            if (ret) {
                LOGE(TAG, "Remove the firmware %d faild %d", g_smartliving_image_idx, ret);
            }

            g_ota_from_smartliving = 0;
        }

        get_version_str(version, ble_node_version);
        app_smartliving_fota_version_rpt(ble_node_version);
    }
    return ret;
}

static int add_ota_node(char *node_image)
{
    if (!node_image) {
        return -1;
    }

    int ret = 0;
    firmware_info info;
    uint8_t ota_num = 0;

    for (int i = 1; i < MAX_DEVICES_META_NUM + 1; i++) {
        if (gateway_subdev_ota_map[i] == 1) {
            // TODO: call mesh ota api
            ota_num++;
        }
    }

    device_info *devices = NULL;

    if (ota_num > 0) {
        devices = (device_info *)aos_zalloc(ota_num * sizeof(device_info));

        if (!devices) {
            LOGE(TAG, "malooc dev info faild");
            aos_free(devices);

            return -1;
        }
    } else {
        LOGE(TAG, "no ota dev found\r\n");
        return -1;
    }

    uint8_t ota_dev_index = 0;
    struct bt_mesh_node_t *node = NULL;

    for (int i = 1; i < MAX_DEVICES_META_NUM + 1; i++) {
        if (gateway_subdev_ota_map[i] == 1) {
            uint8_t mac[6] = {0};
            str2bt_addr(gateway_subdev_info->subdev_info[i].linkkit_dev_meta_info.device_name, mac);
            gateway_mesh_mac_convert(mac, devices[ota_dev_index].addr.val);
            devices[ota_dev_index].addr.type  = 0x00;
            ota_dev_index++;
			dev_addr_t addr = {0};
	        memcpy(&addr,&devices[ota_dev_index].addr,sizeof(mac_t));
            node = bt_mesh_provisioner_get_node_info_by_mac(addr);
            if(!node) {
                LOGE(TAG,"Get node by mac faild");
                devices[ota_dev_index].old_version = 0;
            } else {
                devices[ota_dev_index].old_version = node->version;
            }

        }
    }

    info.address = (uint8_t *)node_image;
    info.image_type = 0x00;
    info.image_pos  = IMAGE_POS_RAM;
    info.size    = dfu_get_fota_file_size(info.address, info.image_pos);
    info.version = dfu_get_fota_file_app_version(info.address, info.image_pos);
    info.ota_chanel = OTA_BY_GATT;
    info.cb = smartliving_ota_firmware_event_cb;

    if (info.size < 0) {
        LOGE(TAG, "read fota image fail\r\n");
        aos_free(devices);
        return -1;
    } else {
        LOGE(TAG, "fota image size %d\r\n", info.size);
    }

    int g_smartliving_image_idx  =  ota_server_upgrade_firmware_add(info);

    if (g_smartliving_image_idx < 0) {
        LOGE(TAG, "Add OTA firmware faild");
        aos_free(devices);
        return -1;
    }

    ret = ota_server_upgrade_device_add(g_smartliving_image_idx, ota_num, devices);

    if (ret) {
        LOGE(TAG, "add ota dev faild");
        aos_free(devices);
        return -1;
    }

    aos_free(devices);

    return 0;
}

int HAL_Firmware_Persistence_Stop(void)
{
    void *ota_handle = NULL;
    int res = SUCCESS_RETURN;
    // uint32_t file_isvalid = 0;

    /* Get Ota Handle */
    res = dm_ota_get_ota_handle(&ota_handle);

    if (res != SUCCESS_RETURN) {
        return FAIL_RETURN;
    }

    if (ota_handle == NULL) {
        return FAIL_RETURN;
    }

    if (IOT_OTA_IsFetchFinish(ota_handle) != 1) {
        LOGE(TAG, "ota fail since file fetch not finished");
        return FAIL_RETURN;
    }

    if (g_ota_for_dev_type == OTA_FOR_GATEWAY) {
        g_ota_from_smartliving = 1;
        return gateway_ota_gateway_upgrade(smartliving_ota_image, smartliving_ota_image_size, smartliving_ota_firmware_event_cb);
    } else if (g_ota_for_dev_type == OTA_FOR_BLE_NODE) {
        g_ota_from_smartliving = 1;
        return add_ota_node(smartliving_ota_image);
    }

    return 0;
}

int HAL_Firmware_Persistence_Write(char *buffer, uint32_t length)
{
    memcpy(smartliving_ota_image + smartliving_ota_image_size, buffer, length);
    smartliving_ota_image_size += length;

    return 0;
}

int app_smartliving_fota_version_rpt(char *version)
{
    void *ota_handle = NULL;
    int res = 0;
    /* Get Ota Handle */
    res = dm_ota_get_ota_handle(&ota_handle);

    if (res != SUCCESS_RETURN) {
        return FAIL_RETURN;
    }

    IOT_OTA_ReportVersion(ota_handle, version);

    return SUCCESS_RETURN;
}

static void smartliving_ota_dev_event_cb(ota_device_state_en event_id, void *param)
{
    // uint8_t version_str[20] = {0};
    // int ret = 0;

    if (!smartliving_ota_image) {
        return;
    }

    uint8_t firmware_id = *(uint8_t *)param;

    if (firmware_id != g_smartliving_image_idx) {
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

        if (status->channel == OTA_BY_GATT) {
            app_smartliving_node_upgrade_check(status->dev_info.addr.val, status->new_ver);
        }

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

static void smartliving_ota_firmware_event_cb(ota_firmware_state_en event, void *data)
{
    ota_firmware_state_data *event_data = (ota_firmware_state_data *)data;

    if (g_smartliving_image_idx != event_data->firmware_index) {
        return;
    }

    switch (event) {
    case FIRMWARE_STATE_IN_USE: {
        LOGD(TAG, "OTA firmware index :%02x in use", event_data->firmware_index);
    }
    break;

    case FIRMWARE_STATE_END_USE: {
        LOGD(TAG, "OTA firmware index :%02x end use", event_data->firmware_index);
    }
    break;

    case FIRMWARE_STATE_REMOVED: {
        LOGD(TAG, "OTA firmware index :%02x removed", event_data->firmware_index);

        if (smartliving_ota_image) {
            free(smartliving_ota_image);
            g_smartliving_image_idx = -1;
            smartliving_ota_image = NULL;
        }
    }
    break;
    default:
    break;
    }
}
