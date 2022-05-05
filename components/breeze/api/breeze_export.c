/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "core.h"
#include "transport.h"
#include "breeze_export.h"
#include "breeze_hal_ble.h"
#include "breeze_hal_os.h"
#include "bzopt.h"

static dev_status_changed_cb m_status_handler;
static set_dev_status_cb m_ctrl_handler;
static get_dev_status_cb m_query_handler;

#if BZ_ENABLE_COMBO_NET
static apinfo_ready_cb m_apinfo_handler;
#endif

#if BZ_ENABLE_OTA
static ota_dev_cb m_ota_dev_handler;
static bool g_disconnect_by_ota = false;
#endif

struct adv_data_s {
    uint8_t data[MAX_VENDOR_DATA_LEN];
    uint32_t len;
} user_adv = {{0}};

static void notify_status(breeze_event_t event)
{
    if (m_status_handler != NULL) {
        m_status_handler(event);
    }
}

static void event_handler(ali_event_t *p_event)
{
    uint32_t err_code;
    bool b_notify_upper = false;
#if BZ_ENABLE_OTA
    breeze_otainfo_t m_disc_evt;
#endif

    switch (p_event->type) {
        case BZ_EVENT_CONNECTED:
            notify_status(CONNECTED);
#if BZ_ENABLE_OTA
            g_disconnect_by_ota = false;
#endif
            break;

        case BZ_EVENT_DISCONNECTED:
            core_reset();
            notify_status(DISCONNECTED);
#if BZ_ENABLE_OTA
            m_disc_evt.type = OTA_EVT;
            m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_DISCONNECTED;
            m_disc_evt.cmd_evt.m_evt.d = 0;
            b_notify_upper = true;
            if(g_disconnect_by_ota == true){
                //do nothing here as expected
            }
#endif
            break;

        case BZ_EVENT_AUTHENTICATED:
            notify_status(AUTHENTICATED);
#if BZ_ENABLE_OTA
            m_disc_evt.type = OTA_EVT;
            m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_AUTH_EVT;
            m_disc_evt.cmd_evt.m_evt.d = 1;
            b_notify_upper = true;
#endif
            break;

        case BZ_EVENT_TX_DONE:
            notify_status(TX_DONE);
#if BZ_ENABLE_OTA
            uint8_t cmd = *p_event->rx_data.p_data;
            if (cmd == BZ_CMD_OTA_CHECK_RESULT || cmd == BZ_CMD_ERR || cmd == BZ_CMD_OTA_PUB_SIZE) {
                m_disc_evt.type = OTA_EVT;
                m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_TX_DONE;
                m_disc_evt.cmd_evt.m_evt.d = cmd;
                b_notify_upper = true;
            }

            /*there is a special case here to handle, for the last disconnected event caused by OTA, 
            * advertising will be confusing, which will be postponed till reboot*/
            if(cmd ==BZ_CMD_OTA_CHECK_RESULT){
                g_disconnect_by_ota = true;
            }
#endif
            break;

        case BZ_EVENT_RX_INFO:
            if(p_event->rx_data.p_data != NULL){
                struct rx_cmd_post_t *r_cmd  = (struct rx_cmd_post_t*) p_event->rx_data.p_data;
                uint8_t cmd = r_cmd ->cmd;	
                if(cmd == BZ_CMD_REQUEST){
                    if (m_query_handler != NULL) {
                        m_query_handler(r_cmd->p_rx_buf, r_cmd->buf_sz);
                    }
                } else if(cmd == BZ_CMD_CTRL){
                    if (m_ctrl_handler != NULL) {
                        m_ctrl_handler (r_cmd->p_rx_buf, r_cmd->buf_sz);
                    }
                }else if((cmd & BZ_CMD_TYPE_MASK) == BZ_CMD_TYPE_OTA){
#if BZ_ENABLE_OTA
                    m_disc_evt.type = OTA_CMD;
                    m_disc_evt.cmd_evt.m_cmd.cmd = r_cmd->cmd;
                    m_disc_evt.cmd_evt.m_cmd.frame = r_cmd->frame_seq;
                    m_disc_evt.cmd_evt.m_cmd.len = r_cmd->buf_sz;
                    memcpy(m_disc_evt.cmd_evt.m_cmd.data, r_cmd->p_rx_buf, r_cmd->buf_sz);
                    b_notify_upper = true;
#endif
                }
            }
            break;

        case BZ_EVENT_APINFO:
#if BZ_ENABLE_COMBO_NET
            if(m_apinfo_handler != NULL){
                m_apinfo_handler(p_event->rx_data.p_data);
	        }
#endif
            break;

        case BZ_EVENT_AC_AS:
            if (p_event->rx_data.p_data != NULL) {
                if ( (p_event->rx_data.p_data[0] == BZ_AC_AS_ADD) 
                    || (p_event->rx_data.p_data[0] == BZ_AC_AS_UPDATE)) {
                    notify_status(EVT_USER_BIND);
                } else if (p_event->rx_data.p_data[0] == BZ_AC_AS_DELETE) {
                    notify_status(EVT_USER_UNBIND);
                }
            }
            break;

        case BZ_EVENT_AUTH_SIGN:
            if (p_event->rx_data.p_data != NULL) {
                if (p_event->rx_data.p_data[0] == BZ_AUTH_SIGN_NO_CHECK_PASS) {
                    // do nothing, for future use
                } else if (p_event->rx_data.p_data[0] == BZ_AUTH_SIGN_CHECK_PASS) {
                    notify_status(EVT_USER_SIGNED);
                }
            }
            break;
        
#if BZ_ENABLE_OTA
        case BZ_EVENT_ERR_DISCONT:
            m_disc_evt.type = OTA_EVT;
            m_disc_evt.cmd_evt.m_evt.evt = ALI_OTA_ON_DISCONTINUE_ERR;
            m_disc_evt.cmd_evt.m_evt.d = 0;
            b_notify_upper = true;
	    break;
#endif
        default:
            break;
    }
#if BZ_ENABLE_OTA
    if(b_notify_upper && (m_ota_dev_handler != NULL)){
        m_ota_dev_handler(&m_disc_evt);
    }
#endif
}

int breeze_start(struct device_config *dev_conf)
{
    uint32_t err_code;
    ali_init_t init_ali;

    if ((dev_conf == NULL) || (dev_conf->status_changed_cb == NULL) ||
        (dev_conf->set_cb == NULL) || (dev_conf->get_cb == NULL)) {
        return -1;
    }
    m_status_handler = dev_conf->status_changed_cb;
    m_ctrl_handler   = dev_conf->set_cb;
    m_query_handler  = dev_conf->get_cb;

#if BZ_ENABLE_COMBO_NET
    if (dev_conf->apinfo_cb != NULL) {
        m_apinfo_handler = dev_conf->apinfo_cb;
    } else {
        return -1;
    }
#endif

#if BZ_ENABLE_OTA
    if (dev_conf->ota_cb != NULL) {
        m_ota_dev_handler = dev_conf->ota_cb;
    } else {
        return -1
    }
#endif

    memset(&init_ali, 0, sizeof(ali_init_t));
    init_ali.event_handler         = event_handler;
    init_ali.model_id              = dev_conf->product_id;
    init_ali.product_key.p_data    = (uint8_t *)dev_conf->product_key;
    init_ali.product_key.length    = dev_conf->product_key_len;
    init_ali.product_secret.p_data = (uint8_t *)dev_conf->product_secret;
    init_ali.product_secret.length = dev_conf->product_secret_len;
    init_ali.device_name.p_data    = (uint8_t *)dev_conf->device_name;
    init_ali.device_name.length    = dev_conf->device_key_len;
    init_ali.device_secret.p_data  = (uint8_t *)dev_conf->device_secret;
    init_ali.device_secret.length  = dev_conf->device_secret_len;
    init_ali.adv_mac               = dev_conf->bd_adv_addr;
    init_ali.transport_timeout     = BZ_TRANSPORT_TIMEOUT;
    init_ali.max_mtu               = BZ_MAX_SUPPORTED_MTU;
    init_ali.user_adv_data         = user_adv.data;
    init_ali.user_adv_len          = user_adv.len;

    err_code = core_init(&init_ali);
    return ((err_code == BZ_SUCCESS) ? 0 : -1);
}

int breeze_end(void)
{
    int ret = 0;

    if (ble_stack_deinit() != AIS_ERR_SUCCESS) {
        ret = -1;
    }

    return 0;
}

uint32_t breeze_post(uint8_t *buffer, uint32_t length)
{
    BREEZE_DEBUG("breeze_post");
    return transport_tx(TX_INDICATION, BZ_CMD_REPORT, buffer, length);
}


uint32_t breeze_post_fast(uint8_t *buffer, uint32_t length)
{
    BREEZE_DEBUG("breeze_post_fast");
    return transport_tx(TX_NOTIFICATION, BZ_CMD_REPORT, buffer, length);
}

uint32_t breeze_post_ext(uint8_t cmd, uint8_t *buffer, uint32_t length)
{
    BREEZE_DEBUG("breeze_post_ext");
    if (length == 0 || length > BZ_MAX_PAYLOAD_SIZE) {
        return BZ_EDATASIZE;
    }

    if (cmd == 0) {
        cmd = BZ_CMD_REPORT;
    }
    return transport_tx(TX_INDICATION, cmd, buffer, length);
}

uint32_t breeze_post_ext_fast(uint8_t cmd, uint8_t *buffer, uint32_t length)
{
    BREEZE_DEBUG("breeze_post_ext_fast");
    if (length == 0 || length > BZ_MAX_PAYLOAD_SIZE) {
        return BZ_EDATASIZE;
    }

    if (cmd == 0) {
        cmd = BZ_CMD_REPORT;
    }
    return transport_tx(TX_NOTIFICATION, cmd, buffer, length);
}

void breeze_append_adv_data(uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0 || len > MAX_VENDOR_DATA_LEN) {
        BREEZE_ERR("invalid adv data");
        return;
    }

    memcpy(user_adv.data, data, len);
    user_adv.len = len;
}

void breeze_restart_advertising()
{
    ais_err_t err;
    uint32_t size;

    ais_adv_init_t adv_data = {
        .flag = AIS_AD_GENERAL | AIS_AD_NO_BREDR,
        .name = { .ntype = AIS_ADV_NAME_FULL, .name = BZ_BT_DEVICE_NAME },
    };

    err = ble_advertising_stop();
    if (err != AIS_ERR_SUCCESS) {
        BREEZE_ERR("Failed to stop previous adv");
        return;
    }

    adv_data.vdata.len = sizeof(adv_data.vdata.data);
    err = core_get_bz_adv_data(adv_data.vdata.data, &(adv_data.vdata.len));
    if (err) {
        BREEZE_ERR("%s %d fail", __func__, __LINE__);
        return;
    }

    if (user_adv.len > 0) {
        size = sizeof(adv_data.vdata.data) - adv_data.vdata.len;
        if (size < user_adv.len) {
            BREEZE_ERR("no space for user adv data (expected %d but"
                   " only %d left)", user_adv.len, size);
        } else {
            memcpy(adv_data.vdata.data + adv_data.vdata.len,
                   user_adv.data, user_adv.len);
            adv_data.vdata.len += user_adv.len;
        }
    }

    ble_advertising_start(&adv_data);
}

int breeze_start_advertising(uint8_t bind_state, uint8_t awss_flag)
{
    uint32_t size;
    ais_adv_init_t adv_data = {
        .flag = AIS_AD_GENERAL | AIS_AD_NO_BREDR,
        .name = { .ntype = AIS_ADV_NAME_FULL, .name = BZ_BT_DEVICE_NAME },
    };
    
    core_create_bz_adv_data(bind_state, awss_flag);

    adv_data.vdata.len = sizeof(adv_data.vdata.data);
    if (core_get_bz_adv_data(adv_data.vdata.data, &(adv_data.vdata.len))) {
        BREEZE_ERR("%s %d fail", __func__, __LINE__);
        return -1;
    }

    /* append user adv data if any. */
    if (user_adv.len > 0) {
        size = sizeof(adv_data.vdata.data) - adv_data.vdata.len;
        if (size < user_adv.len) {
            BREEZE_ERR("no space for user adv data (expected %d but"
                   " only %d left)", user_adv.len, size);
        } else {
            memcpy(adv_data.vdata.data + adv_data.vdata.len,
                   user_adv.data, user_adv.len);
            adv_data.vdata.len += user_adv.len;
        }
    }

    if (ble_advertising_start(&adv_data) != AIS_ERR_SUCCESS) {
        BREEZE_ERR("%s %d adv fail", __func__, __LINE__);
        return -1;
    }
    return 0;
}

int breeze_stop_advertising(void)
{
    if (ble_advertising_stop() != AIS_ERR_SUCCESS) {
        BREEZE_ERR("stop adv fail");
        return -1;
    }
    return 0;
}

void breeze_disconnect_ble(void)
{
    ble_disconnect(AIS_BT_REASON_REMOTE_USER_TERM_CONN);
}

uint8_t breeze_get_bind_state(void)
{
    uint8_t kv_data[16 + 32 + 2] = {0};
    int kv_len = sizeof(kv_data);
    if (os_kv_get(BZ_AUTH_CODE_KV_PREFIX, kv_data, &kv_len) != 0) {
        BREEZE_DEBUG("no AC get from kv");
        return 0;
    } else {
        BREEZE_VERBOSE("AC from kv:");
        hex_byte_dump_verbose(kv_data, kv_len, 24);
        return 1;
    }
}

int breeze_clear_bind_info(void)
{
    int ret = 0;
    if (os_kv_del(BZ_AUTH_CODE_KV_PREFIX) != 0) {
        BREEZE_ERR("AC&AS clear failed");
        ret = -1;
    }
    return ret;
}
