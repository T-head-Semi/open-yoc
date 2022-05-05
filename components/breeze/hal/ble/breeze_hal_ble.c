/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#if (defined CONFIG_BREEZE_HAL_BLE_BLUEDROID) && CONFIG_BREEZE_HAL_BLE_BLUEDROID		
#include <stdio.h>		
#include <breeze_hal_ble.h>		
#include "yoc_gap_ble_api.h"		
#include "yoc_gatts_api.h"		
#include "bta/bta_api.h"		
#include "aos/kernel.h"		
#include <aos/list.h>		
#include "breeze_hal_os.h"		
#include <ulog/ulog.h>		
#include "yoc_gatt_common_api.h"


#define TAG "Breeze_hal"		
#define COMBONET_APP_ID 0		
#define PROFILE_COMBO_NET_NUM 1		
#define PROFILE_COMBO_NET_APP_ID 0		
#define GATT_DATA_MAX_LEN (512)		
enum {		
    COMBO_IDX_SVC,		
    RC_CHAR,		
    RC_VAL,		
    WC_CHAR,		
    WC_VAL,		
    IC_CHAR,		
    IC_VAL,		
    IC_CCC,		
    WWNRC_CHAR,		
    WWNRC_VAL,		
    NC_CHAR,		
    NC_VAL,		
    NC_CCC,		
    COMBO_IDX_NB,		
};		
ais_bt_init_t *bt_init_info = NULL;		
static void (*g_indication_txdone)(uint8_t res);		
static int breeze_conn_flag = 0;		
yoc_bd_addr_t remote_addr;		
void          ble_disconnect(uint8_t reason)		
{		
    if(breeze_conn_flag){		
        printf("Active disconnect reason %d\n", reason);		
        yoc_ble_gap_disconnect(remote_addr);		
        breeze_conn_flag = 0;		
    }		
}		
static void connected(uint8_t err)		
{		
    if (err) {		
        printf("Connection failed (err %u)\n", err);		
    } else {		
        printf("Connected\n");		
        if (bt_init_info && (bt_init_info->on_connected)) {		
            bt_init_info->on_connected();		
        }		
    }		
}		
static void disconnected(uint8_t reason)		
{		
    printf("Disconnected (reason %u)\n", reason);		
    if (bt_init_info && (bt_init_info->on_disconnected)) {		
        bt_init_info->on_disconnected();		
    }		
}		
static void ais_nc_ccc_cfg_changed(uint16_t value)		
{		
    ais_ccc_value_t val;		
    switch (value) {		
        case 1:		
            printf("CCC cfg changed to NOTIFY (%d).\r\n", value);		
            val = AIS_CCC_VALUE_NOTIFY;		
            break;		
        default:		
            printf("%s CCC cfg changed to %d.\r\n", __func__, value);		
            val = AIS_CCC_VALUE_NONE;		
            break;		
    }		
    if (bt_init_info && bt_init_info->nc.on_ccc_change) {		
        bt_init_info->nc.on_ccc_change(val);		
    }		
}		
static void ais_ic_ccc_cfg_changed(uint16_t value)		
{		
    ais_ccc_value_t val;		
    switch (value) {		
        case 2:		
            printf("CCC cfg changed to INDICATE (%d).\r\n", value);		
            val = AIS_CCC_VALUE_INDICATE;		
            break;		
        default:		
            printf("%s CCC cfg changed to %d.\r\n", __func__, value);		
            val = AIS_CCC_VALUE_NONE;		
            break;		
    }		
    if (bt_init_info && bt_init_info->ic.on_ccc_change) {		
        bt_init_info->ic.on_ccc_change(val);		
    }		
}		
static int write_ais_wc(void *buf, uint16_t len)		
{		
    int ret = 0;		
    if (bt_init_info && bt_init_info->wc.on_write) {		
        ret = bt_init_info->wc.on_write(buf, len);		
    }		
    return ret;		
}		
static uint8_t ble_adv_config_done = 0;		
//static uint8_t ble_set_local_privacy_done = 0;		
#define ble_adv_config_flag (1 << 0)		
#define ble_scan_rsp_config_flag (1 << 1)		
static int ble_inited = 0;		
static yoc_ble_adv_params_t adv_params = {		
    .adv_int_min       = 0x20,		
    .adv_int_max       = 0x25,		
    .adv_type          = ADV_TYPE_IND,		
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,		
    .channel_map       = ADV_CHNL_ALL,		
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,		
};		
// The length of adv data must be less than 31 bytes		
//adv data		
static yoc_ble_adv_data_t adv_data = {		
    .set_scan_rsp     = false,		
    .include_name     = false,		
    .include_txpower  = false,		
    .min_interval     = 0, //slave connection min interval, Time = min_interval * 1.25 msec		
    .max_interval     = 0, //slave connection max interval, Time = max_interval * 1.25 msec		
    .appearance       = 0x00,		
    .service_data_len = 0,		
    .p_service_data   = NULL,		
};		
// scan response data		
static yoc_ble_adv_data_t scan_rsp_data = {		
    .set_scan_rsp        = true,		
    .include_name        = true,		
    .include_txpower     = true,		
    .min_interval        = 0, //0x0006		
    .max_interval        = 0, // 0x0010		
    .appearance          = 0,		
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,		
    .p_manufacturer_data = NULL, //&test_manufacturer[0],		
    .service_data_len    = 0,		
    .p_service_data      = NULL,		
    // .service_uuid_len = 16,		
    // .p_service_uuid = adv_service_uuid128,		
    .flag = (YOC_BLE_ADV_FLAG_GEN_DISC | YOC_BLE_ADV_FLAG_BREDR_NOT_SPT),		
};		
static void gap_event_handler(yoc_gap_ble_cb_event_t event, yoc_ble_gap_cb_param_t *param)		
{		
    switch (event) {		
        case YOC_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:		
            ble_adv_config_done &= (~ble_adv_config_flag);		
            if (ble_adv_config_done == 0) {		
                ble_inited = 1;		
                LOGD(TAG, "BLE GAP inited");		
            }		
            break;		
        case YOC_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:		
            ble_adv_config_done &= (~ble_scan_rsp_config_flag);		
            if (ble_adv_config_done == 0) {		
                ble_inited = 1;		
                LOGD(TAG, "BLE GAP inited");		
            }		
            break;		
        case YOC_GAP_BLE_ADV_START_COMPLETE_EVT:		
            //advertising start complete event to indicate advertising start successfully or failed		
            if (param->adv_start_cmpl.status != YOC_BT_STATUS_SUCCESS)		
                LOGE(TAG, "Advertising start failed\n");		
            break;		
        case YOC_GAP_BLE_ADV_STOP_COMPLETE_EVT:		
            if (param->adv_stop_cmpl.status != YOC_BT_STATUS_SUCCESS) {		
                LOGE(TAG, "Advertising stop failed\n");		
            } else {		
                LOGI(TAG, "Stop adv successfully\n");		
            }		
            break;		
        case YOC_GAP_BLE_UPDATE_CONN_PARAMS_EVT:		
            LOGI(TAG,		
                 "update connection params status = %d, min_int = %d, max_int = %d,conn_int = "		
                 "%d,latency = %d, timeout = %d",		
                 param->update_conn_params.status, param->update_conn_params.min_int,		
                 param->update_conn_params.max_int, param->update_conn_params.conn_int,		
                 param->update_conn_params.latency, param->update_conn_params.timeout);		
            break;		
        //case YOC_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:		
        //    ble_set_local_privacy_done = 1;		
        //    break;		
        default:		
            break;		
    }		
}		
#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))		
static const uint16_t primary_service_uuid         = YOC_GATT_UUID_PRI_SERVICE;		
static const uint16_t character_declaration_uuid   = YOC_GATT_UUID_CHAR_DECLARE;		
static const uint16_t character_client_config_uuid = YOC_GATT_UUID_CHAR_CLIENT_CONFIG;		
static const uint8_t char_prop_read = YOC_GATT_CHAR_PROP_BIT_READ;		
static const uint8_t char_prop_read_indicate =		
    YOC_GATT_CHAR_PROP_BIT_READ | YOC_GATT_CHAR_PROP_BIT_INDICATE;		
static const uint8_t char_prop_read_notify =		
    YOC_GATT_CHAR_PROP_BIT_READ | YOC_GATT_CHAR_PROP_BIT_NOTIFY;		
static const uint8_t char_prop_read_write =		
    YOC_GATT_CHAR_PROP_BIT_WRITE | YOC_GATT_CHAR_PROP_BIT_READ;		
static const uint8_t char_prop_read_write_nr =		
    YOC_GATT_CHAR_PROP_BIT_WRITE_NR | YOC_GATT_CHAR_PROP_BIT_READ;		
static const uint16_t service_uuid = BLE_UUID_AIS_SERVICE;		
static const uint16_t data_write_uuid     = BLE_UUID_AIS_WC;		
static const uint8_t  data_write_val[100] = {0x00};		
static const uint16_t data_write_not_response_uuid     = BLE_UUID_AIS_WWNRC;		
static const uint8_t  data_write_not_response_val[100] = {0x00};		
static const uint16_t data_read_uuid    = BLE_UUID_AIS_RC;		
static const uint8_t  data_read_val[20] = {0x00};		
static const uint16_t data_notify_uuid    = BLE_UUID_AIS_NC;		
static const uint8_t  data_notify_val[20] = {0x00};		
static const uint8_t  data_notify_ccc[2]  = {0x00, 0x00};		
static const uint16_t data_indicate_uuid    = BLE_UUID_AIS_IC;		
static const uint8_t  data_indicate_val[20] = {0x00};		
static const uint8_t  data_indicate_ccc[2]  = {0x00, 0x00};		
static uint16_t                  combo_net_gatt_handle_table[COMBO_IDX_NB];		
static const yoc_gatts_attr_db_t gatt_db[COMBO_IDX_NB] = {		
    [COMBO_IDX_SVC] = {{YOC_GATT_AUTO_RSP},		
                       {YOC_UUID_LEN_16, (uint8_t *)&primary_service_uuid, YOC_GATT_PERM_READ,		
                        sizeof(service_uuid), sizeof(service_uuid), (uint8_t *)&service_uuid}},		
    [RC_CHAR] = {{YOC_GATT_AUTO_RSP},		
                 {YOC_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, YOC_GATT_PERM_READ,		
                  CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},		
    [RC_VAL] = {{YOC_GATT_AUTO_RSP},		
                {YOC_UUID_LEN_16, (uint8_t *)&data_read_uuid, YOC_GATT_PERM_READ, GATT_DATA_MAX_LEN,		
                 sizeof(data_read_val), (uint8_t *)data_read_val}},		
    [WC_CHAR] = {{YOC_GATT_AUTO_RSP},		
                 {YOC_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, YOC_GATT_PERM_READ,		
                  CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},		
    [WC_VAL] = {{YOC_GATT_AUTO_RSP},		
                {YOC_UUID_LEN_16, (uint8_t *)&data_write_uuid,		
                 YOC_GATT_PERM_READ | YOC_GATT_PERM_WRITE, GATT_DATA_MAX_LEN,		
                 sizeof(data_write_val), (uint8_t *)data_write_val}},		
    [IC_CHAR] = {{YOC_GATT_AUTO_RSP},		
                 {YOC_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,		
                  YOC_GATT_PERM_READ | YOC_GATT_PERM_WRITE, CHAR_DECLARATION_SIZE,		
                  CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_indicate}},		
    [IC_VAL] = {{YOC_GATT_AUTO_RSP},		
                {YOC_UUID_LEN_16, (uint8_t *)&data_indicate_uuid, YOC_GATT_PERM_READ,		
                 GATT_DATA_MAX_LEN, sizeof(data_indicate_val), (uint8_t *)data_indicate_val}},		
    [IC_CCC] = {{YOC_GATT_AUTO_RSP},		
                {YOC_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,		
                 YOC_GATT_PERM_READ | YOC_GATT_PERM_WRITE, sizeof(uint16_t),		
                 sizeof(data_indicate_ccc), (uint8_t *)data_indicate_ccc}},		
    [WWNRC_CHAR] = {{YOC_GATT_AUTO_RSP},		
                    {YOC_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, YOC_GATT_PERM_READ,		
                     CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,		
                     (uint8_t *)&char_prop_read_write_nr}},		
    [WWNRC_VAL] = {{YOC_GATT_AUTO_RSP},		
                   {YOC_UUID_LEN_16, (uint8_t *)&data_write_not_response_uuid,		
                    YOC_GATT_PERM_READ | YOC_GATT_PERM_WRITE, GATT_DATA_MAX_LEN,		
                    sizeof(data_write_not_response_val), (uint8_t *)data_write_not_response_val}},		
    [NC_CHAR] = {{YOC_GATT_AUTO_RSP},		
                 {YOC_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,		
                  YOC_GATT_PERM_READ | YOC_GATT_PERM_WRITE, CHAR_DECLARATION_SIZE,		
                  CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},		
    [NC_VAL] = {{YOC_GATT_AUTO_RSP},		
                {YOC_UUID_LEN_16, (uint8_t *)&data_notify_uuid,		
                 YOC_GATT_PERM_READ | YOC_GATT_PERM_WRITE, GATT_DATA_MAX_LEN,		
                 sizeof(data_notify_val), (uint8_t *)data_notify_val}},		
    [NC_CCC] = {{YOC_GATT_AUTO_RSP},		
                {YOC_UUID_LEN_16, (uint8_t *)&character_client_config_uuid,		
                 YOC_GATT_PERM_READ | YOC_GATT_PERM_WRITE, sizeof(uint16_t),		
                 sizeof(data_notify_ccc), (uint8_t *)data_notify_ccc}},		
};		
static uint8_t find_char_and_desr_index(uint16_t handle)		
{		
    uint8_t error = 0xff;		
    for (int i = 0; i < COMBO_IDX_NB; i++) {		
        if (handle == combo_net_gatt_handle_table[i]) {		
            return i;		
        }		
    }		
    return error;		
}		
struct gatts_profile_inst {		
    yoc_gatts_cb_t       gatts_cb;		
    uint16_t             gatts_if;		
    uint16_t             app_id;		
    uint16_t             conn_id;		
    uint16_t             service_handle;		
    yoc_gatt_srvc_id_t   service_id;		
    yoc_gatt_perm_t      perm;		
    yoc_gatt_char_prop_t property;		
};		
static void gatts_profile_a_event_handler(yoc_gatts_cb_event_t event, yoc_gatt_if_t gatts_if,		
                                          yoc_ble_gatts_cb_param_t *param);		
static struct gatts_profile_inst profile_combo_net_tab[PROFILE_COMBO_NET_NUM] = {		
    [PROFILE_COMBO_NET_APP_ID] =		
        {		
            .gatts_cb = gatts_profile_a_event_handler,		
            .gatts_if = YOC_GATT_IF_NONE, /* Not get the gatt_if, so initial is YOC_GATT_IF_NONE */		
        },		
};		
static uint16_t      gatt_mtu_size  = 100;		
static uint16_t      gatts_connid   = 0xffff;		
static yoc_gatt_if_t combo_gatts_if = 0xff;		
static void gatts_profile_a_event_handler(yoc_gatts_cb_event_t event, yoc_gatt_if_t gatts_if,		
                                          yoc_ble_gatts_cb_param_t *param)		
{		
    uint8_t res = 0xff;		
    LOGI(TAG, "event = %d gatts_if = %d\n", event, gatts_if);		
    switch (event) {		
        case YOC_GATTS_REG_EVT:		
            LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status,		
                 param->reg.app_id);		
            yoc_ble_gatts_create_attr_tab(gatt_db, gatts_if, COMBO_IDX_NB, 0);		
            break;		
        case YOC_GATTS_CREAT_ATTR_TAB_EVT:		
            LOGI(TAG, "The number handle =%x\n", param->add_attr_tab.num_handle);		
            if (param->add_attr_tab.status != YOC_GATT_OK) {		
                LOGE(TAG, "Create attribute table failed, error code=0x%x",		
                     param->add_attr_tab.status);		
            } else if (param->add_attr_tab.num_handle != COMBO_IDX_NB) {		
                LOGE(TAG,		
                     "Create attribute table abnormally, num_handle (%d) doesn't equal to "		
                     "HRS_IDX_NB(%d)",		
                     param->add_attr_tab.num_handle, COMBO_IDX_NB);		
            } else {		
                memcpy(combo_net_gatt_handle_table, param->add_attr_tab.handles,		
                       sizeof(combo_net_gatt_handle_table));		
                yoc_ble_gatts_start_service(combo_net_gatt_handle_table[COMBO_IDX_SVC]);		
            }		
            break;		
        case YOC_GATTS_CONNECT_EVT:		
            LOGI(TAG,		
                 "YOC_GATTS_CONNECT_EVT, gatts_if %d conn_id %d, remote "		
                 "%02x:%02x:%02x:%02x:%02x:%02x:",		
                 gatts_if, param->connect.conn_id, param->connect.remote_bda[0],		
                 param->connect.remote_bda[1], param->connect.remote_bda[2],		
                 param->connect.remote_bda[3], param->connect.remote_bda[4],		
                 param->connect.remote_bda[5]);		
            gatts_connid   = param->connect.conn_id;		
            combo_gatts_if = gatts_if;		
            memcpy(remote_addr, param->connect.remote_bda, sizeof(yoc_bd_addr_t));		
            breeze_conn_flag = 1;		
            profile_combo_net_tab[PROFILE_COMBO_NET_APP_ID].conn_id = param->connect.conn_id;		
            connected(0);		
            break;		
        case YOC_GATTS_DISCONNECT_EVT:		
            LOGI(TAG, "YOC_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);		
            disconnected(param->disconnect.reason);		
            breeze_conn_flag = 0;		
            break;		
        case YOC_GATTS_READ_EVT:		
            LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id,		
                 param->read.trans_id, param->read.handle);		
            break;		
        case YOC_GATTS_WRITE_EVT:		
            LOGI(TAG,		
                 "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d len:%d offset:%d response:%d "		
                 "is_prep %d",		
                 param->write.conn_id, param->write.trans_id, param->write.handle, param->write.len,		
                 param->write.offset, param->write.need_rsp, param->write.is_prep);		
            res = find_char_and_desr_index(param->write.handle);		
            if (param->write.is_prep == false) {		
                LOGI(TAG, "YOC_GATTS_WRITE_EVT : handle = %d\n", res);		
                if (res == NC_CCC) {		
                    if (param->write.len == 2) {		
                        uint16_t descr_value_nc =		
                            param->write.value[1] << 8 | param->write.value[0];		
                        ais_nc_ccc_cfg_changed(descr_value_nc);		
                    }		
                } else if (res == IC_CCC) {		
                    if (param->write.len == 2) {		
                        uint16_t descr_value_ic =		
                            param->write.value[1] << 8 | param->write.value[0];		
                        ais_ic_ccc_cfg_changed(descr_value_ic);		
                    }		
                } else if (res == WC_VAL) {		
                    write_ais_wc(param->write.value, param->write.len);		
                }		
            }		
            break;		
        case YOC_GATTS_MTU_EVT:		
            LOGI(TAG, "GATT_MTU_EVT, conn_id %d, mtu %d\n", param->mtu.conn_id, param->mtu.mtu);		
            gatt_mtu_size = param->mtu.mtu;		
            break;		
        case YOC_GATTS_CONF_EVT:		
            g_indication_txdone(0);		
            break;		
        default:		
            break;		
    }		
}		
static void gatts_event_handler(yoc_gatts_cb_event_t event, yoc_gatt_if_t gatts_if,		
                                yoc_ble_gatts_cb_param_t *param)		
{		
    /* If event is register event, store the gatts_if for each profile */		
    if (event == YOC_GATTS_REG_EVT) {		
        if (param->reg.status == YOC_GATT_OK) {		
            profile_combo_net_tab[param->reg.app_id].gatts_if = gatts_if;		
        } else {		
            LOGI(TAG, "Reg app failed, app_id %04x, status %d\n", param->reg.app_id,		
                 param->reg.status);		
            return;		
        }		
    }		
    /* If the gatts_if equal to profile A, call profile A cb handler,		
     * so here call each profile's callback */		
    do {		
        int idx;		
        for (idx = 0; idx < PROFILE_COMBO_NET_NUM; idx++) {		
            if (gatts_if ==		
                    YOC_GATT_IF_NONE || /* YOC_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */		
                gatts_if == profile_combo_net_tab[idx].gatts_if) {		
                if (profile_combo_net_tab[idx].gatts_cb) {		
                    profile_combo_net_tab[idx].gatts_cb(event, gatts_if, param);		
                }		
            }		
        }		
    } while (0);		
}		
static int ble_stack_init = 0;		
ais_err_t  bz_ble_stack_init(ais_bt_init_t *info)		
{		
    bt_err_t ret;		
    bt_init_info = info;		
    if (!ble_stack_init) {		
        ret = yoc_ble_gap_register_callback(gap_event_handler);		
        if (ret) {		
            LOGE(TAG, "gap register error, error code = %x", ret);		
            return AIS_ERR_STACK_FAIL;		
        }		
        ret = yoc_ble_gatts_register_callback(gatts_event_handler);		
        if (ret) {		
            LOGE(TAG, "gatts register error, error code = %x", ret);		
            return AIS_ERR_STACK_FAIL;		
        }		
        ret = yoc_ble_gatts_app_register(COMBONET_APP_ID);		
        if (ret) {		
            LOGE(TAG, "gatts app register error, error code = %x", ret);		
            return AIS_ERR_STACK_FAIL;		
        }		
        ble_stack_init = 1;		
    }		
    return AIS_ERR_SUCCESS;		
}		
ais_err_t ble_stack_deinit()		
{		
    if (ble_stack_init) {		
        yoc_ble_gatts_app_unregister(combo_gatts_if);		
        ble_stack_init = 0;		
    }		
    return AIS_ERR_SUCCESS;		
}		
ais_err_t ble_send_notification(uint8_t *p_data, uint16_t length)		
{		
    int ret;		
    ret = yoc_ble_gatts_send_indicate(combo_gatts_if, gatts_connid,		
                                      combo_net_gatt_handle_table[NC_VAL], length, p_data, false);		
    return ret;		
}		
ais_err_t ble_send_indication(uint8_t *p_data, uint16_t length, void (*txdone)(uint8_t res))		
{		
    int ret;		
    g_indication_txdone = txdone;		
    ret                 = yoc_ble_gatts_send_indicate(combo_gatts_if, gatts_connid,		
                                      combo_net_gatt_handle_table[IC_VAL], length, p_data, true);		
    return ret;		
}		
static uint8_t adv_service_uuid128[16] = {		
    /*12 13*/		
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xb3, 0xfe, 0x00, 0x00,		
};		
static int breeze_advertising_flag = 0;		
ais_err_t ble_advertising_start(ais_adv_init_t *adv)		
{		
    bt_err_t ret = BT_OK;		
    if(!breeze_advertising_flag) {		
        //yoc_ble_gap_config_local_privacy(true);		
        // bt_err_t set_dev_name_ret = yoc_ble_gap_set_device_name(adv->name.name);		
        // if (set_dev_name_ret) {		
        //     LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);		
        // }		
        adv_data.flag = adv->flag;		
        if (adv->vdata.len != 0) {		
            adv_data.manufacturer_len    = adv->vdata.len;		
            adv_data.p_manufacturer_data = adv->vdata.data;		
        } else {		
            adv_data.manufacturer_len    = 0;		
            adv_data.p_manufacturer_data = NULL;		
        }		
        adv_data.service_uuid_len = YOC_UUID_LEN_128;		
        adv_data.p_service_uuid   = adv_service_uuid128;		
        ret = yoc_ble_gap_config_adv_data(&adv_data);		
        if (ret) {		
            LOGE(TAG, "config adv data failed, error code = %x", ret);		
        }		
        ble_adv_config_done |= ble_adv_config_flag;		
        ret = yoc_ble_gap_config_adv_data(&scan_rsp_data);		
        if (ret) {		
            LOGE(TAG, "config scan response data failed, error code = %x", ret);		
        }		
        ble_adv_config_done |= ble_scan_rsp_config_flag;		
        /* wait till bt inited */		
        for (int i = 0; i < 25 && !ble_inited; i++) {		
            aos_msleep(200);		
        }		
        if (!ble_inited) {		
            LOGE(TAG, "start when bt not inited");		
            return -1;		
        }		
        //while(!ble_set_local_privacy_done);		
        ret = yoc_ble_gap_start_advertising(&adv_params);		
        //ble_set_local_privacy_done = 0;		
        breeze_advertising_flag = 1;		
        LOGE(TAG, "ble start advertising");		
    }		
    return ret == BT_OK ? AIS_ERR_SUCCESS : AIS_ERR_ADV_FAIL;		
}		
ais_err_t ble_advertising_stop()		
{		
    int ret = BT_OK;		
    if(breeze_advertising_flag) {		
        ret = yoc_ble_gap_stop_advertising();		
        breeze_advertising_flag = 0;		
    }		
    return ret == BT_OK ? AIS_ERR_SUCCESS : AIS_ERR_STOP_ADV_FAIL;		
}		
ais_err_t ble_get_mac(uint8_t *mac)		
{		
    ais_err_t err = AIS_ERR_SUCCESS;		
    bt_err_t  ret;		
    uint8_t   addr_type = 0;		
    ret                 = yoc_ble_gap_get_local_used_addr(mac, &addr_type);		
    if (ret != BT_OK)		
        LOGD(TAG, "ble_get_mac error");		
    return err;		
}		
#ifdef EN_LONG_MTU		
int ble_get_att_mtu(uint16_t *att_mtu)		
{		
    if (att_mtu == NULL) {		
        printf("Failed to get ble connection\r\n");		
        return -1;		
    }		
    *att_mtu = gatt_mtu_size;		
    return 0;		
}		
#endif		
#else
#include <stdio.h>
#include <breeze_hal_ble.h>
#include <misc/slist.h>
#include <ble_config.h>
#include <ble_os.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include "aos/kernel.h"
#include <aos/list.h>
#include "breeze_hal_os.h"
#include "aos/ble.h"
#include "host/conn_internal.h"

struct bt_conn *g_conn = NULL;
ais_bt_init_t * bt_init_info = NULL;

#define BT_UUID_AIS_SERVICE BT_UUID_DECLARE_16(BLE_UUID_AIS_SERVICE)
#define BT_UUID_AIS_RC BT_UUID_DECLARE_16(BLE_UUID_AIS_RC)
#define BT_UUID_AIS_WC BT_UUID_DECLARE_16(BLE_UUID_AIS_WC)
#define BT_UUID_AIS_IC BT_UUID_DECLARE_16(BLE_UUID_AIS_IC)
#define BT_UUID_AIS_WWNRC BT_UUID_DECLARE_16(BLE_UUID_AIS_WWNRC)
#define BT_UUID_AIS_NC BT_UUID_DECLARE_16(BLE_UUID_AIS_NC)

#define TAG "Breeze"


static struct bt_gatt_ccc_cfg ais_ic_ccc_cfg[BT_GATT_CCC_MAX] = {};
static struct bt_gatt_ccc_cfg ais_nc_ccc_cfg[BT_GATT_CCC_MAX] = {};

static void (*g_indication_txdone)(uint8_t res);

void ble_disconnect(uint8_t reason)
{
    uint8_t zreason = 0;

    if (!g_conn) {
        return;
    }
    printf("Active disconnect reason %d\n", reason);

    switch (reason) {
        case AIS_BT_REASON_REMOTE_USER_TERM_CONN:
            zreason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
            break;
        default:
            zreason = BT_HCI_ERR_UNSPECIFIED;
            break;
    }

    bt_conn_disconnect(g_conn, zreason);
}


static void connected(struct bt_conn *conn, uint8_t err)
{
    if(conn->role != BT_HCI_ROLE_SLAVE) {
        return;
    }

    if (err) {
        printf("Connection failed (err %u)\n", err);
    } else {
        printf("Connected\n");
        g_conn = conn;
        if (bt_init_info && (bt_init_info->on_connected)) {
            bt_init_info->on_connected();
        }
    }
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
    if(conn->role != BT_HCI_ROLE_SLAVE) {
        return;
    }
    printf("Disconnected (reason %u)\n", reason);
    g_conn = NULL;
    if (bt_init_info && (bt_init_info->on_disconnected)) {
        bt_init_info->on_disconnected();
    }
}

static void ais_nc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                   uint16_t                   value)
{
    ais_ccc_value_t val;

    switch (value) {
        case BT_GATT_CCC_NOTIFY:
            printf("CCC cfg changed to NOTIFY (%d).\r\n", value);
            val = AIS_CCC_VALUE_NOTIFY;
            break;
        default:
            printf("%s CCC cfg changed to %d.\r\n", __func__, value);
            val = AIS_CCC_VALUE_NONE;
            break;
    }

    if (bt_init_info && bt_init_info->nc.on_ccc_change) {
        bt_init_info->nc.on_ccc_change(val);
    }
}

static void ais_ic_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                   uint16_t                   value)
{
    ais_ccc_value_t val;

    switch (value) {
        case BT_GATT_CCC_INDICATE:
            printf("CCC cfg changed to INDICATE (%d).\r\n", value);
            val = AIS_CCC_VALUE_INDICATE;
            break;
        default:
            printf("%s CCC cfg changed to %d.\r\n", __func__, value);
            val = AIS_CCC_VALUE_NONE;
            break;
    }

    if (bt_init_info && bt_init_info->ic.on_ccc_change) {
        bt_init_info->ic.on_ccc_change(val);
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected    = connected,
    .disconnected = disconnected,
};

static ssize_t read_ais_rc(struct bt_conn *           conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
    ssize_t ret = 0;

    printf("%s length to read: %d, offset: %d\r\n", __func__, len, offset);
    if (bt_init_info && bt_init_info->rc.on_read) {
        ret = bt_init_info->rc.on_read(buf, len);
    }

    return ret;
}

static ssize_t read_ais_wc(struct bt_conn *           conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
    ssize_t ret = 0;

    printf("%s length to read: %d, offset: %d\r\n", __func__, len, offset);
    if (bt_init_info && bt_init_info->wc.on_read) {
        ret = bt_init_info->wc.on_read(buf, len);
    }

    return ret;
}

static ssize_t write_ais_wc(struct bt_conn *           conn,
                            const struct bt_gatt_attr *attr, const void *buf,
                            uint16_t len, uint16_t offset, uint8_t flags)
{
    ssize_t ret = 0;

    if (bt_init_info && bt_init_info->wc.on_write) {
        ret = bt_init_info->wc.on_write(buf, len);
    }

    return ret;
}

static ssize_t read_ais_ic(struct bt_conn *           conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
    ssize_t ret = 0;

    printf("%s length to read: %d, offset: %d\r\n", __func__, len, offset);
    if (bt_init_info && bt_init_info->ic.on_read) {
        ret = bt_init_info->ic.on_read(buf, len);
    }

    return ret;
}

static ssize_t read_ais_wwnrc(struct bt_conn *           conn,
                              const struct bt_gatt_attr *attr, void *buf,
                              uint16_t len, uint16_t offset)
{
    ssize_t ret = 0;

    printf("%s length to read: %d, offset: %d\r\n", __func__, len, offset);
    if (bt_init_info && bt_init_info->wwnrc.on_read) {
        ret = bt_init_info->wwnrc.on_read(buf, len);
    }

    return ret;
}

static ssize_t write_ais_wwnrc(struct bt_conn *           conn,
                               const struct bt_gatt_attr *attr, const void *buf,
                               uint16_t len, uint16_t offset, uint8_t flags)
{
    ssize_t ret = 0;

    if (bt_init_info && bt_init_info->wwnrc.on_write) {
        ret = bt_init_info->wwnrc.on_write(buf, len);
    }

    return ret;
}

static ssize_t read_ais_nc(struct bt_conn *           conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
    ssize_t ret = 0;

    printf("%s length to read: %d, offset: %d\r\n", __func__, len, offset);
    if (bt_init_info && bt_init_info->nc.on_read) {
        ret = bt_init_info->nc.on_read(buf, len);
    }

    return ret;
}

static struct bt_gatt_attr *  ais_attrs = NULL;
static struct bt_gatt_service ais_svc;

struct bt_uuid *bt_prisvc_uuid = BT_UUID_GATT_PRIMARY;
struct bt_uuid *bt_chrc_uuid   = BT_UUID_GATT_CHRC;
struct bt_uuid *bt_ccc_uuid    = BT_UUID_GATT_CCC;

static int setup_ais_service_attr(struct bt_gatt_attr *attr,
                                  struct bt_uuid *     uuid)
{
    attr->uuid      = bt_prisvc_uuid;
    attr->perm      = BT_GATT_PERM_READ;
    attr->read      = bt_gatt_attr_read_service;
    attr->user_data = (void *)uuid;
    return 0;
}

static int setup_ais_char_attr(struct bt_gatt_attr *attr, struct bt_uuid *uuid,
                               uint8_t prop)
{
    struct bt_gatt_chrc *chrc = NULL;

    chrc = (struct bt_gatt_chrc *)aos_malloc(sizeof(struct bt_gatt_chrc));
    if (!chrc) {
        printf("%s malloc failed\r\n", __func__);
        return -1;
    }

    memset(chrc, 0, sizeof(struct bt_gatt_chrc));

    chrc->uuid       = uuid;
    chrc->properties = prop;

    attr->uuid      = bt_chrc_uuid;
    attr->perm      = BT_GATT_PERM_READ;
    attr->read      = bt_gatt_attr_read_chrc;
    attr->user_data = (void *)chrc;

    return 0;
}

static int setup_ais_char_desc_attr(struct bt_gatt_attr *attr,
                                    struct bt_uuid *uuid, uint8_t perm,
                                    void *read, void *write, void *userdata)
{
    attr->uuid      = uuid;
    attr->perm      = perm;
    attr->read      = read;
    attr->write     = write;
    attr->user_data = userdata;
    return 0;
}

static int setup_ais_char_ccc_attr(struct bt_gatt_attr *   attr,
                                   struct bt_gatt_ccc_cfg *cfg, size_t cfg_len,
                                   void *cfg_handler)
{
    struct _bt_gatt_ccc *ccc = NULL;

    ccc = (struct _bt_gatt_ccc *)aos_malloc(sizeof(struct _bt_gatt_ccc));
    if (!ccc) {
        printf("%s malloc failed.\r\n", __func__);
        return -1;
    }
    memset(ccc, 0, sizeof(struct _bt_gatt_ccc));

    //ccc->cfg         = cfg;
    //ccc->cfg_len     = cfg_len;
    memcpy((u8_t*)ccc->cfg, cfg, sizeof(struct bt_gatt_ccc_cfg)*cfg_len);
    ccc->cfg_changed = cfg_handler;

    attr->uuid      = bt_ccc_uuid;
    attr->perm      = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE;
    attr->read      = bt_gatt_attr_read_ccc;
    attr->write     = bt_gatt_attr_write_ccc;
    attr->user_data = (void *)ccc;

    return 0;
}

enum
{
    SVC_ATTR_IDX = 0,
    RC_CHRC_ATTR_IDX,
    RC_DESC_ATTR_IDX,
    WC_CHRC_ATTR_IDX,
    WC_DESC_ATTR_IDX,
    IC_CHRC_ATTR_IDX,
    IC_DESC_ATTR_IDX,
    IC_CCC_ATTR_IDX,
    WWNRC_CHRC_ATTR_IDX,
    WWNRC_DESC_ATTR_IDX,
    NC_CHRC_ATTR_IDX,
    NC_DESC_ATTR_IDX,
    NC_CCC_ATTR_IDX,
    /* Add more entry here if necessary */
    AIS_ATTR_NUM,
};

ais_err_t bz_ble_stack_init(ais_bt_init_t *info)
{
    int              err;
    uint32_t         attr_cnt = AIS_ATTR_NUM, size;
    ais_char_init_t *c;

    bt_init_info = info;

    err = bt_enable(NULL);
    if (err && err != -EALREADY) {
        printf("Bluetooth init failed (err %d)\n", err);
        return AIS_ERR_STACK_FAIL;
    }

    bt_set_name(BZ_BT_DEVICE_NAME);

    bt_setup_public_id_addr();

    bt_finalize_init();

    printf("Bluetooth init succeed.\n");

    size      = attr_cnt * sizeof(struct bt_gatt_attr);
    ais_attrs = (struct bt_gatt_attr *)aos_malloc(size);
    if (!ais_attrs) {
        printf("%s %d memory allocate failed.\r\n", __func__, __LINE__);
        return AIS_ERR_MEM_FAIL;
    }

    memset(ais_attrs, 0, size);

    /* AIS primary service */
    setup_ais_service_attr(&ais_attrs[SVC_ATTR_IDX], info->uuid_svc);

    /* rc */
    c = &(info->rc);
    setup_ais_char_attr(&ais_attrs[RC_CHRC_ATTR_IDX], c->uuid, c->prop);
    setup_ais_char_desc_attr(&ais_attrs[RC_DESC_ATTR_IDX], c->uuid, c->perm,
                             read_ais_rc, NULL, NULL);

    /* wc */
    c = &(info->wc);
    setup_ais_char_attr(&ais_attrs[WC_CHRC_ATTR_IDX], c->uuid, c->prop);
    setup_ais_char_desc_attr(&ais_attrs[WC_DESC_ATTR_IDX], c->uuid, c->perm,
                             read_ais_wc, write_ais_wc, NULL);

    /* ic */
    c = &(info->ic);
    setup_ais_char_attr(&ais_attrs[IC_CHRC_ATTR_IDX], c->uuid, c->prop);
    setup_ais_char_desc_attr(&ais_attrs[IC_DESC_ATTR_IDX], c->uuid, c->perm,
                             read_ais_ic, NULL, NULL);
    setup_ais_char_ccc_attr(&ais_attrs[IC_CCC_ATTR_IDX], ais_ic_ccc_cfg,
                            sizeof(ais_ic_ccc_cfg) / sizeof(ais_ic_ccc_cfg[0]),
                            ais_ic_ccc_cfg_changed);

    /* wwnrc */
    c = &(info->wwnrc);
    setup_ais_char_attr(&ais_attrs[WWNRC_CHRC_ATTR_IDX], c->uuid, c->prop);
    setup_ais_char_desc_attr(&ais_attrs[WWNRC_DESC_ATTR_IDX], c->uuid, c->perm,
                             read_ais_wwnrc, write_ais_wwnrc, NULL);

    /* nc */
    c = &(info->nc);
    setup_ais_char_attr(&ais_attrs[NC_CHRC_ATTR_IDX], c->uuid, c->prop);
    setup_ais_char_desc_attr(&ais_attrs[NC_DESC_ATTR_IDX], c->uuid, c->perm,
                             read_ais_nc, NULL, NULL);
    setup_ais_char_ccc_attr(&ais_attrs[NC_CCC_ATTR_IDX], ais_nc_ccc_cfg,
                            sizeof(ais_nc_ccc_cfg) / sizeof(ais_nc_ccc_cfg[0]),
                            ais_nc_ccc_cfg_changed);

    memset(&ais_svc, 0, sizeof(ais_svc));
    ais_svc.attrs      = ais_attrs;
    ais_svc.attr_count = attr_cnt;

    bt_conn_cb_register(&conn_callbacks);
    bt_gatt_service_register(&ais_svc);
    return AIS_ERR_SUCCESS;
}

ais_err_t ble_stack_deinit()
{
    if (ais_attrs) {
        aos_free(ais_attrs);
        ais_attrs = NULL;
    }

    /* Free other memory here when necessary. */

    return AIS_ERR_SUCCESS;
}

ais_err_t ble_send_notification(uint8_t *p_data, uint16_t length)
{
    return bt_gatt_notify(NULL, &ais_attrs[NC_DESC_ATTR_IDX],
                          (const void *)p_data, length);
}

slist_t params_list;

typedef struct bt_gatt_indicate_param_s
{
    int                             wp;
    int                             rp;
    int                             size;
    struct bt_gatt_indicate_params *ind_params;
} gatt_param_buf_t;

static gatt_param_buf_t g_gatt_params;

static void indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        uint8_t err)
{
    struct bt_gatt_indicate_params *param;
    int rp = g_gatt_params.rp;
    int wp = g_gatt_params.wp;

    if (rp != wp) {
        param = &g_gatt_params.ind_params[rp];
        memset(param, 0, sizeof(struct bt_gatt_indicate_params));
        g_gatt_params.rp = (rp + 1) % g_gatt_params.size;
    }

    g_indication_txdone(0);
}

ais_err_t ble_send_indication(uint8_t *p_data, uint16_t length, void (*txdone)(uint8_t res))
{
    int err;
    struct bt_gatt_indicate_params *ind_params;
    int wp, rp;

    if (!g_gatt_params.ind_params) {
        g_gatt_params.wp = 0;
        g_gatt_params.rp = 0;
        g_gatt_params.size = 4;
        g_gatt_params.ind_params = aos_zalloc_check(g_gatt_params.size * sizeof(struct bt_gatt_indicate_params));
    }

    wp = g_gatt_params.wp;
    rp = g_gatt_params.rp;

    /*ringbuf is full*/
    if ((wp + 1) % g_gatt_params.size == rp) {
        printf("Warn: No indication tx buffer.\n");
        return AIS_ERR_MEM_FAIL;
    }

    ind_params = &g_gatt_params.ind_params[wp];
    ind_params->attr = &ais_attrs[IC_DESC_ATTR_IDX];
    ind_params->func = indicate_cb;
    ind_params->data = p_data;
    ind_params->len  = length;

    err = bt_gatt_indicate(NULL, ind_params);
    if (err) {
        memset(ind_params, 0, sizeof(struct bt_gatt_indicate_params));
        return AIS_ERR_GATT_INDICATE_FAIL;
    } 

    g_gatt_params.wp = (wp + 1) % g_gatt_params.size;
    g_indication_txdone = txdone;
    return AIS_ERR_SUCCESS;
}

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, BZ_BT_DEVICE_NAME,
            sizeof(BZ_BT_DEVICE_NAME) - 1),
};

ais_err_t ble_advertising_start(ais_adv_init_t *adv)
{
    int            err;
    uint8_t        flag = 0, srv[] = { 0xb3, 0xfe }, ad_len = 3, sd_len = 1;
    struct bt_data ad[ad_len];
    struct bt_data sd[sd_len];

    if (adv->flag & AIS_AD_GENERAL) {
        flag |= BT_LE_AD_GENERAL;
    }
    if (adv->flag & AIS_AD_NO_BREDR) {
        flag |= BT_LE_AD_NO_BREDR;
    }
    if (!flag) {
        printf("Invalid adv flag.\r\n");
        return AIS_ERR_INVALID_ADV_DATA;
    }

    ad[0].type     = BT_DATA_FLAGS;
    ad[0].data     = &flag;
    ad[0].data_len = 1;

    ad[1].type     = BT_DATA_UUID16_ALL;
    ad[1].data     = srv;
    ad[1].data_len = sizeof(srv);

    if (adv->vdata.len != 0) {
        ad[2].type     = 0xFF;
        ad[2].data_len = adv->vdata.len;
        ad[2].data     = adv->vdata.data;
    } else {
        ad_len--;
    }

    switch (adv->name.ntype) {
        case AIS_ADV_NAME_SHORT:
            sd[0].type = BT_DATA_NAME_SHORTENED;
            break;
        case AIS_ADV_NAME_FULL:
            sd[0].type = BT_DATA_NAME_COMPLETE;
            break;
        default:
            printf("Invalid adv name type.\r\n");
            return AIS_ERR_INVALID_ADV_DATA;
    }

    if (adv->name.name == NULL) {
        printf("Invalid adv device name.\r\n");
        return AIS_ERR_INVALID_ADV_DATA;
    }

    sd[0].data     = adv->name.name;
    sd[0].data_len = strlen(adv->name.name);
    struct bt_le_adv_param p = *BT_LE_ADV_CONN;
    p.options |= BT_LE_ADV_OPT_ONE_TIME;
    // p.options |= BT_LE_ADV_OPT_USE_IDENTITY;
    p.id = 0;
    err = bt_le_adv_start(&p, ad, ad_len, sd, sd_len);
    if (err) {
        printf("Advertising failed to start (err %d)\n", err);
        return AIS_ERR_ADV_FAIL;
    }

    return 0;
}

ais_err_t ble_advertising_stop()
{
    int ret;
    ret = bt_le_adv_stop();
    return ret ? AIS_ERR_STOP_ADV_FAIL : 0;
}

ais_err_t ble_get_mac(uint8_t *mac)
{
    ais_err_t    err = AIS_ERR_SUCCESS;
    bt_addr_le_t laddr = {0};
    struct bt_le_oob    oob = {0};

    //err = ais_ota_get_local_addr(&laddr);
    err = bt_le_oob_get_local(0, &oob);
    if (err != AIS_ERR_SUCCESS) {
        printf("Failed to get local addr.\r\n");
    } else {
        memcpy(mac, oob.addr.a.val, 6);
        printf("Local addr got (%02x:%02x:%02x:%02x:%02x:%02x).\n", mac[0],
               mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    return err;
}

#ifdef EN_LONG_MTU
int ble_get_att_mtu(uint16_t *att_mtu)
{
    if(att_mtu == NULL || g_conn == NULL){
        printf("Failed to get ble connection\r\n");
        return -1;
    }
    *att_mtu = bt_gatt_get_mtu(g_conn);
    return 0;
}
#endif
#endif

