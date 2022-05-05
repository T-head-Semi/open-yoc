/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */
#include "dfu_port.h"
#include "ais_ota.h"
#include "ais_ota/ais_ota_client.h"
#include "ota_module.h"
#include "ota_trans/ota_client_trans.h"
#include "ulog/ulog.h"
#include "ota_version.h"
#include "errno.h"

#ifdef	CONFIG_BT_MESH_MODEL_VENDOR_SRV
#include "ble_os.h"
#include "vendor/vendor_model_srv.h"
#endif

#define TAG  "AIS_OTA_CLIENT"

ais_ota_ctx_t ais_ota_ctx;


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static const char *log_hex(const void *buf, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    static char str[256];
    const uint8_t *b = buf;
    size_t i;

    len = MIN(len, (sizeof(str) - 1) / 2);

    for (i = 0; i < len; i++) {
        str[i * 2]     = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;

}

void ais_ota_crc_check_ctl(uint8_t flag)
{
    ais_ota_ctx.crc_check_flag = flag;
}

void ais_ota_auto_reboot_ctl(uint8_t flag)
{
    ais_ota_ctx.auto_reboot_flag = flag;
}

bool ais_ota_is_ready(void)
{
    return ais_ota_ctx.ota_ready;
}

bool ais_ota_get_indication(void)
{
    return 0;
}

static int _ais_ota_state_set(uint8_t state)
{
    ais_ota_ctx.state = state;

    return 0;
}

static uint8_t _ais_ota_state_get(void)
{
    return ais_ota_ctx.state;
}

static void _ais_set_ota_change(void)
{
    ais_ota_ctx.ota_ready = 1;
}

static int _ais_ota_client_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack)
{
    return ota_client_trans_send(ais_ota_ctx.channel, msg_id, cmd, p_msg, len, ack);
}

static int _ais_ota_client_disconnect()
{
    return ota_client_trans_disconnect(ais_ota_ctx.channel);
}

static void _ais_ota_status_report(void)
{
    uint8_t plaine_data[OTA_CRYPTO_UNIT_SIZE];

    ais_ota_status_report_t *p_status_report = (ais_ota_status_report_t *)plaine_data;

    memset(plaine_data, 11, sizeof(plaine_data));
    p_status_report->last_seq = ais_ota_ctx.last_seq;
    p_status_report->total_frame = ais_ota_ctx.total_frame;
    p_status_report->rx_size = ais_ota_ctx.rx_size;

    LOGD(TAG,"last[%d] total[%d] size[%d] err[%d]", p_status_report->last_seq, p_status_report->total_frame, p_status_report->rx_size, ais_ota_ctx.err_count);

    _ais_ota_client_send(0, AIS_OTA_STATUS, plaine_data, OTA_CRYPTO_UNIT_SIZE, 0);
}

static bool _ais_ota_handle_version_request(uint8_t msg_id, ais_ota_ver_req_t *p_ver_req)
{
    uint8_t plaine_data[OTA_CRYPTO_UNIT_SIZE];

    ais_ota_ver_resp_t *p_ver_resp = (ais_ota_ver_resp_t *)plaine_data;

    if (p_ver_req->image_type == 0) {
        memset(plaine_data, 11, sizeof(plaine_data));
        p_ver_resp->image_type = 0;
        p_ver_resp->ver = ota_version_appver_get();

        if (_ais_ota_state_get() == AIS_STATE_IDLE) {
            _ais_ota_client_send(msg_id, AIS_OTA_VER_RESP, plaine_data, 5, 0);
        } else {
            _ais_ota_client_send(msg_id, AIS_OTA_VER_RESP, plaine_data, 5, 0); //5=sizeof(ais_ota_ver_resp_t)
        }

        return true;
    }

    return false;
}

static bool _ais_ota_handle_update_request(uint8_t msg_id, ais_ota_upd_req_t *p_ota_req)
{
    uint8_t plaine_data[OTA_CRYPTO_UNIT_SIZE];

    ais_ota_upd_resp_t *p_upd_resp = (ais_ota_upd_resp_t *)plaine_data;

    LOGD(TAG,"%d ota_ver %08x %08x size %d misc size: %d %d",p_ota_req->image_type, p_ota_req->ver,ota_version_appver_get(),p_ota_req->fw_size, dfu_get_ota_partition_max_size(),ais_ota_ctx.ota_ready);
    if (ais_ota_ctx.ota_ready == 1) {
        return false;
    }
    memset(plaine_data, 10, sizeof(plaine_data));
#ifdef CONFIG_ALLOW_OTA_FOR_HIGH_VERSION
    if (p_ota_req->image_type != 0 || p_ota_req->ver == ota_version_appver_get() || \
        dfu_get_ota_partition_max_size() < p_ota_req->fw_size || 0 == p_ota_req->fw_size) //Is illeagal
#else
    if (p_ota_req->image_type != 0 || p_ota_req->ver <= ota_version_appver_get() || \
        dfu_get_ota_partition_max_size() < p_ota_req->fw_size || 0 == p_ota_req->fw_size)
#endif

    {
        LOGE(TAG,"ver:0x%08x imgT:%d imgS:%d", p_ota_req->ver, p_ota_req->image_type, p_ota_req->fw_size);
        p_upd_resp->state = 0;
        p_upd_resp->rx_size = 0;
    } else {
        //memset(&ais_ota_ctx, 0, sizeof(ais_ota_ctx));
        _ais_ota_state_set(AIS_STATE_OTA);
        ais_ota_ctx.image_type = p_ota_req->image_type;
        ais_ota_ctx.image_ver = p_ota_req->ver;
        ais_ota_ctx.image_size = p_ota_req->fw_size;
        ais_ota_ctx.image_crc16 = p_ota_req->crc16;
        ais_ota_ctx.ota_flag = p_ota_req->ota_flag;
        p_upd_resp->state = 1;
        /*If support breakpoint continue,you should set rx_size*/
        p_upd_resp->rx_size = 0;
    }

    p_upd_resp->total_frame = CONFIG_AIS_TOTAL_FRAME - 1;
    _ais_ota_client_send(msg_id, AIS_OTA_UPD_RESP, plaine_data, OTA_CRYPTO_UNIT_SIZE, 0);

    return true;
}

static bool _ais_ota_parse_pdu(ais_pdu_t *p_msg)
{
    int ret = 0 ;
    uint8_t *p_payload = p_msg->payload;
    uint16_t payload_len = p_msg->header.payload_len;

    if (p_msg->header.seq > p_msg->header.total_frame) {
        LOGE(TAG,"invalid");
        return false;
    }

    if (p_msg->header.seq != ais_ota_ctx.except_seq) {
        LOGW(TAG,"expected %d, rx %d, len %d", ais_ota_ctx.except_seq, p_msg->header.seq, p_msg->header.payload_len);
        if (ais_ota_ctx.err_count++ == 0) {
            /* send fail */
            _ais_ota_status_report();
            /* refresh timer */
            return true;
        }
        return false;
    }

    LOGD(TAG,"b4:rx %d/%d", ais_ota_ctx.rx_size, ais_ota_ctx.image_size);
    if (ais_ota_ctx.rx_size + p_msg->header.payload_len > ais_ota_ctx.image_size) {
        LOGE(TAG,"out of size, rx %ld, recv %d", ais_ota_ctx.rx_size, p_msg->header.payload_len);
        return false;
    }

    ais_ota_ctx.err_count = 0;

    if (p_msg->header.seq == 0) {
        ais_ota_ctx.rx_len = 0;
        memset(ais_ota_ctx.recv_buf, 0, sizeof(ais_ota_ctx.recv_buf));
    }

    if (payload_len) {
        //LOGD(TAG,"save %d", payload_len);
        ais_ota_ctx.flash_clean = 1;
        if((ais_ota_ctx.rx_len + payload_len) > sizeof(ais_ota_ctx.recv_buf)) {
			LOGE(TAG,"Out of recv buffer %d %d %d",ais_ota_ctx.rx_len,payload_len,sizeof(ais_ota_ctx.recv_buf));
            return false;
        }

        memcpy(ais_ota_ctx.recv_buf + ais_ota_ctx.rx_len, p_payload, payload_len);
        ais_ota_ctx.rx_len += payload_len;
    }

    ais_ota_ctx.last_seq = p_msg->header.seq;
    ais_ota_ctx.total_frame = p_msg->header.total_frame;
    ais_ota_ctx.rx_size += p_msg->header.payload_len;
    LOGD(TAG,"rx %d/%d", ais_ota_ctx.rx_size, ais_ota_ctx.image_size);
    if (p_msg->header.seq == p_msg->header.total_frame) {
        ret = dfu_image_update(ais_ota_ctx.image_type,
                               ais_ota_ctx.rx_size - ais_ota_ctx.rx_len, ais_ota_ctx.rx_len,
                               (int *)ais_ota_ctx.recv_buf);
        if(ret) {
            LOGE(TAG,"Dfu image update faild");
            return false;
        }
        ais_ota_ctx.rx_len = 0;
        ais_ota_ctx.except_seq = 0;
    } else {
        ais_ota_ctx.except_seq = p_msg->header.seq + 1;
    }

    if (ais_ota_ctx.rx_size == ais_ota_ctx.image_size || p_msg->header.seq == p_msg->header.total_frame) {
        _ais_ota_status_report();
    }

    return true;
}

static bool _ais_ota_check_firmware(uint8_t msg_id, ais_ota_check_req_t *p_check_req)
{
    uint16_t crc16 = 0;
    uint8_t plaine_data[OTA_CRYPTO_UNIT_SIZE];

    ais_ota_check_resp_t *p_check_resp = (ais_ota_check_resp_t *)plaine_data;

    _ais_ota_state_set(AIS_STATE_IDLE);

    if (p_check_req->state == 1) {
        memset(plaine_data, 15, sizeof(plaine_data));

        p_check_resp->state = dfu_check_checksum(ais_ota_ctx.image_type, &crc16);

        LOGD(TAG,"check %d %04x %04x %d", p_check_resp->state, ais_ota_ctx.image_crc16, crc16, ais_ota_ctx.crc_check_flag);
        if(ais_ota_ctx.crc_check_flag) {
            if (p_check_resp->state && crc16 != ais_ota_ctx.image_crc16) {
                p_check_resp->state = 0;
                LOGE(TAG,"crc error");
            }
        }

        if (p_check_resp->state) {
            _ais_set_ota_change();
            if(ais_ota_ctx.auto_reboot_flag) {
                _ais_ota_state_set(AIS_STATE_REBOOT);
                LOGW(TAG,"ota success, reboot in 800ms!");
            } else {
                _ais_ota_state_set(AIS_STATE_WAITREBOOT);
                LOGW(TAG, "ota success, wait reboot");
            }
        } else {
            LOGE(TAG,"ota failed");
        }

        _ais_ota_client_send(msg_id, AIS_OTA_CHECK_RESP, plaine_data, OTA_CRYPTO_UNIT_SIZE, 0);

        return true;
    } else {
        return false;
    }
}





static void _ais_dis_timer_cb(void *p_timer, void *args)
{
    ota_timer_stop(&ais_ota_ctx.disconnect_timer);
    _ais_ota_client_disconnect();
}


static void _ais_ota_state_update(void)
{
    LOGD(TAG,"ais state %d", ais_ota_ctx.state);

    switch (ais_ota_ctx.state) {
    case AIS_STATE_DISCON:
        ota_timer_stop(&ais_ota_ctx.state_update_timer);
        _ais_ota_client_disconnect();
        break;
    case AIS_STATE_CONNECT:
    case AIS_STATE_IDLE:
        //no disconnect after one minute
        //k_timer_start(&ais_ota_ctx.state_update_timer, AIS_OTA_DISCONN_TIMEOUT);
        break;
    case AIS_STATE_OTA:
        ota_timer_stop(&ais_ota_ctx.state_update_timer);
        ota_timer_start(&ais_ota_ctx.state_update_timer,AIS_OTA_REPORT_TIMEOUT);
        break;
    case AIS_STATE_REBOOT:
        ota_timer_stop(&ais_ota_ctx.state_update_timer);
        ota_timer_start(&ais_ota_ctx.state_update_timer,AIS_OTA_REPORT_TIMEOUT);
        ota_timer_stop(&ais_ota_ctx.disconnect_timer);
        ota_timer_start(&ais_ota_ctx.disconnect_timer,AIS_DISCONNECT_TIMEOUT);
        break;
    default:
        break;
    }
}

static bool _ais_msg_check_header(ais_header_t *p_msg_header)
{
    //check seq & total, in ota case, the seq & total must be 0
    if (p_msg_header->total_frame != 0 || p_msg_header->seq != 0 || p_msg_header->ver != 0 || p_msg_header->seq > p_msg_header->total_frame) {
        LOGE(TAG,"fail %s", log_hex(p_msg_header, sizeof(ais_header_t)));
        return false;
    }
    return true;
}


static void state_update_timer_cb(void *p_timer, void *args)
{
    ota_timer_stop(&ais_ota_ctx.state_update_timer);
    LOGI(TAG,"timer cb %d", ais_ota_ctx.state);
    switch (ais_ota_ctx.state) {
    case AIS_STATE_CONNECT:
    case AIS_STATE_IDLE:
        ais_ota_ctx.state = AIS_STATE_DISCON;
        break;
    case AIS_STATE_AUTH:
        ais_ota_ctx.state = AIS_STATE_CONNECT;
        break;
    case AIS_STATE_OTA:
        if (ais_ota_ctx.err_count++ >= OTA_RECV_MAX_ERR_COUNT) {
            LOGE(TAG,"OTA failed");
            ais_ota_ctx.state = AIS_STATE_IDLE;
            ais_ota_ctx.rx_len = 0;
            _ais_ota_client_send(0, AIS_RESP_ERR,NULL, 0, 1);
        } else {
            _ais_ota_status_report();
        }
        break;
    case AIS_STATE_REBOOT:
        dfu_reboot();
        break;
    default:
        break;
    }
    _ais_ota_state_update();
}


static void _ais_ota_client_connect_event()
{
    if(!ais_ota_ctx.init_flag) {
        return;
    }
    if (ais_ota_ctx.state == AIS_STATE_REBOOT) {
        LOGE(TAG,"Ais state reboot,not allowed to connect");
        _ais_ota_client_disconnect();
    } else {
        LOGD(TAG,"status %d", ais_ota_ctx.state);
        ais_ota_ctx.state = AIS_STATE_CONNECT;
        _ais_ota_state_update();
    }
}

static void _ais_ota_client_disconnect_event()
{
    if(!ais_ota_ctx.init_flag) {
        return;
    }

    if (ais_ota_ctx.state != AIS_STATE_REBOOT) {
        ota_timer_stop(&ais_ota_ctx.state_update_timer);
        _ais_ota_client_disconnect();

        ais_ota_ctx.state = AIS_STATE_DISCON;
        /* Flash is dirty, need erase */
        if (ais_ota_ctx.flash_clean == 1 &&
            ais_ota_ctx.ota_ready == 0) {
            erase_dfu_flash(0);
            ais_ota_ctx.flash_clean = 0;
        }
        /* restart adv */
    } else {
        if (ais_ota_ctx.ota_flag != OTA_FLAG_SILENT || sal_ota_is_allow_reboot()) {
            LOGI(TAG,"OTA Reboot!");
            dfu_reboot();
        }
    }
}



static void _ais_server_msg_handle(ais_pdu_t *p_msg, uint16_t len)
{
    bool timer_refresh = false;
    LOGD(TAG,"ais state %d cmd %02x",ais_ota_ctx.state, p_msg->header.cmd);

    if(p_msg->header.cmd != AIS_OTA_DATA) {
        LOGD(TAG,"len %d: %s", len, log_hex(p_msg, len));
    }

    if (p_msg->header.cmd != AIS_OTA_DATA && !_ais_msg_check_header((ais_header_t *)p_msg)) {
        LOGE(TAG,"invalid msg, ignore");
    }

    if (p_msg->header.cmd != AIS_OTA_DATA) {
        LOGI(TAG,"AIS Cmd:0x%02x", p_msg->header.cmd);
    }

    switch (p_msg->header.cmd) {
    case AIS_OTA_VER_REQ:
        if ((len == 20 && ais_ota_ctx.state == AIS_STATE_IDLE) || (len == 5 && ais_ota_ctx.state == AIS_STATE_CONNECT)) {
            timer_refresh = _ais_ota_handle_version_request(p_msg->header.msg_id,
                            (ais_ota_ver_req_t *)p_msg->payload);
            //TODO
            ais_ota_ctx.state = AIS_STATE_IDLE;
        }
        break;

    case AIS_OTA_FIRMWARE_REQ:
        //len = 4+16
        //TODO
        if (len == 16 && ais_ota_ctx.state == AIS_STATE_IDLE) {
            timer_refresh = _ais_ota_handle_update_request(p_msg->header.msg_id,
                            (ais_ota_upd_req_t *)p_msg->payload);
        }
        break;

    case AIS_OTA_DATA:
        if (len == sizeof(ais_header_t) + p_msg->header.payload_len && p_msg->header.ver == 0 && ais_ota_ctx.state == AIS_STATE_OTA) {
            timer_refresh = _ais_ota_parse_pdu(p_msg);
        }
        break;

    case AIS_OTA_CHECK_REQ:
        //TODO len 20->5
        if (len == 5 && ais_ota_ctx.state == AIS_STATE_OTA) {
            timer_refresh = _ais_ota_check_firmware(p_msg->header.msg_id, (ais_ota_check_req_t *)p_msg->payload);
        }
        break;

    default:
        /* recv some unsupport cmd, just return */
        LOGW(TAG,"unsupport cmd %x", p_msg->header.cmd);
        return;
    }

    if (timer_refresh) {
        _ais_ota_state_update();
    } else {
        if (ais_ota_ctx.state != AIS_STATE_OTA) {
            _ais_ota_client_send(p_msg->header.msg_id, AIS_RESP_ERR, NULL, 0, 1);
        }
    }
}

static void ais_ota_client_cb(uint8_t event, const void *event_data, uint8_t len)
{
    LOGD(TAG,"ais ota client event %02x",event);
    switch(event) {
    case OTA_CLIENT_EVENT_CONN: {
        _ais_ota_client_connect_event();
    }
    break;
    case OTA_CLIENT_EVENT_DISCONN: {
        _ais_ota_client_disconnect_event();
    }
    break;
    case OTA_CLIENT_EVENT_WRITE: {
        _ais_server_msg_handle((ais_pdu_t*)event_data, len);
    }
    break;

    default:
        break;
    }
    return;
}


int ais_ota_client_init()
{
    int channel  = 0;

    if(ais_ota_ctx.init_flag) {
        return 0;
    }

    channel = ota_client_trans_init(ais_ota_client_cb);
    if(channel < 0) {
        return -1;
    }
    memset(&ais_ota_ctx, 0, sizeof(ais_ota_ctx_t));
    erase_dfu_flash(0);
    ota_timer_init(&ais_ota_ctx.state_update_timer,state_update_timer_cb, NULL);
    ota_timer_init(&ais_ota_ctx.disconnect_timer,_ais_dis_timer_cb, NULL);
    ais_ota_ctx.flash_clean = 0;
    ais_ota_ctx.channel = channel;
    ais_ota_ctx.init_flag = 1;
    ais_ota_ctx.crc_check_flag = 1;
    ais_ota_ctx.auto_reboot_flag = 1;
    return 0;
}


int ais_ota_report_version(void)
{
#ifdef	CONFIG_BT_MESH_MODEL_VENDOR_SRV
    return ble_mesh_vendor_srv_model_version_report(ota_version_appver_get());
#else
    return -EOPNOTSUPP;
#endif
}


