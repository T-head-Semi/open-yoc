/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if !defined(CONFIG_SMART_AUDIO_NO_BT) || (CONFIG_SMART_AUDIO_NO_BT == 0)

#include <stdlib.h>
#include <aos/kv.h>
#include <ulog/ulog.h>
#include "yoc_bt_main.h"
#include "yoc_gap_bt_api.h"
#include "yoc_a2dp_api.h"
#include "yoc_avrc_api.h"
#include "yoc_gap_ble_api.h"
#include "yoc_bt_device.h"
#include "yoc_app_bt.h"
#include <smart_audio.h>
#include "btc_audio_a2dp_player.h"

#define TAG "smtaudio_ctrl_bt_a2dp"

#define AVRCP_PLAY_STATUS_INQUIRE_TIME 2000

static const char *  s_a2d_conn_state_str[]  = {"Disconnected", "Connecting", "Connected",
                                             "Disconnecting"};
static const char *  s_a2d_audio_state_str[] = {"Suspended", "Stopped", "Started"};
static aos_mutex_t   s_a2dp_mutex;
static aos_timer_t   s_a2dp_inquire_timer;
static int           g_a2dp_connect_state        = YOC_A2D_CONNECTION_STATE_DISCONNECTED;
static unsigned char cur_bt_addr[BT_BD_ADDR_LEN] = {0};

static void                  avrcp_play_status_inquire_timer_start(void);
static void                  avrcp_play_status_inquire_timer_stop(void);
static void                  bt_callback(yoc_app_bt_event_t event, yoc_app_bt_param_t *param);
static yoc_app_bt_callback_t g_bt_cb = NULL;

static int bt_reconnect_flag;

static int bt_a2dp_init(void);
static int bt_a2dp_deinit(void);
static int bt_a2dp_start(const char *url, uint64_t seek_time, int resume);
static int bt_a2dp_pause(void);
static int bt_a2dp_stop(void);
static int bt_a2dp_resume(void);
static int bt_a2dp_vol_get(void);
static int bt_a2dp_vol_set(int vol);
static int bt_a2dp_vol_up(int vol);
static int bt_a2dp_vol_down(int vol);

smtaudio_ops_node_t ctrl_bt_a2dp = {
    .name     = "bt_a2dp",
    .prio     = 2,
    .id       = SMTAUDIO_BT_A2DP,
    .status   = SMTAUDIO_STATE_STOP,
    .init     = bt_a2dp_init,
    .deinit   = bt_a2dp_deinit,
    .start    = bt_a2dp_start,
    .pause    = bt_a2dp_pause,
    .stop     = bt_a2dp_stop,
    .resume   = bt_a2dp_resume,
    .vol_get  = bt_a2dp_vol_get,
    .vol_set  = bt_a2dp_vol_set,
    .vol_up   = bt_a2dp_vol_up,
    .vol_down = bt_a2dp_vol_down,
};

static uint8_t _bt_get_lable(void)
{
    static uint8_t lable = 0;

    lable++;
    lable = lable % 15;

    return lable;
}

static void _bt_gap_cb(yoc_bt_gap_cb_event_t event, yoc_bt_gap_cb_param_t *param)
{
    switch (event) {
    case YOC_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == YOC_BT_STATUS_SUCCESS) {
            LOGD(TAG, "authentication success: %s", param->auth_cmpl.device_name);

            yoc_app_bt_param_t param_m;
            memcpy(param_m.paired.remote_addr, param->auth_cmpl.bda, BT_BD_ADDR_LEN);
            bt_callback(YOC_APP_BT_PAIRED, &param_m);
            if (g_bt_cb) {
                g_bt_cb(YOC_APP_BT_PAIRED, &param_m);
            }
        } else {
            LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case YOC_BT_GAP_CFM_REQ_EVT:
        LOGD(TAG, "YOC_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d",
             param->cfm_req.num_val);
        // YOC_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case YOC_BT_GAP_KEY_NOTIF_EVT:
        LOGD(TAG, "YOC_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case YOC_BT_GAP_KEY_REQ_EVT:
        LOGD(TAG, "YOC_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
    case YOC_BT_GAP_READ_RSSI_DELTA_EVT:
        LOGD(TAG, "rssi delta %d", param->read_rssi_delta.rssi_delta);
        break;
    case YOC_BT_GAP_CONFIG_EIR_DATA_EVT:
        LOGD(TAG, "EIR DATA %d", param->config_eir_data.stat);
        break;
    default: {
        LOGE(TAG, "unhandled event: %d", event);
        break;
    }
    }
    return;
}

static void _bt_app_a2d_cb(yoc_a2d_cb_event_t event, yoc_a2d_cb_param_t *param)
{
    yoc_a2d_cb_param_t *a2d = (yoc_a2d_cb_param_t *)(param);

    LOGD(TAG, "%s evt %d", __func__, event);
    switch (event) {
    case YOC_A2D_CONNECTION_STATE_EVT: {
        uint8_t *bda;

        bda = a2d->conn_stat.remote_bda;
        LOGD(TAG, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
             s_a2d_conn_state_str[a2d->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4],
             bda[5]);
        if (a2d->conn_stat.state == YOC_A2D_CONNECTION_STATE_DISCONNECTED) {
                avrcp_play_status_inquire_timer_stop();
            if (a2d->conn_stat.disc_rsn == YOC_A2D_DISC_RSN_NORMAL) {
                yoc_app_bt_param_t param;
                memcpy(param.a2dp_conn.remote_addr, bda, BT_BD_ADDR_LEN);
                if (bt_reconnect_flag) {
                    LOGD(TAG, "try reconnect");
                    yoc_app_bt_a2dp_connect(cur_bt_addr);
                } else {
                    bt_callback(YOC_APP_BT_A2DP_DISCONNECTED, &param);
                    if (g_bt_cb) {
                        g_bt_cb(YOC_APP_BT_A2DP_DISCONNECTED, &param);
                    }
                }
                g_a2dp_connect_state = YOC_A2D_CONNECTION_STATE_DISCONNECTED;
            } else {
                yoc_app_bt_param_t param;
                memcpy(param.a2dp_conn.remote_addr, bda, BT_BD_ADDR_LEN);
                bt_callback(YOC_APP_BT_A2DP_LINK_LOSS, &param);
                if (g_bt_cb) {
                    g_bt_cb(YOC_APP_BT_A2DP_LINK_LOSS, &param);
                }
                LOGD(TAG, "try reconnect");
                yoc_app_bt_a2dp_connect(cur_bt_addr);
                g_a2dp_connect_state = YOC_A2D_CONNECTION_STATE_DISCONNECTED;
                bt_reconnect_flag    = 1;
            }
        } else if (a2d->conn_stat.state == YOC_A2D_CONNECTION_STATE_CONNECTED) {
            if ((bt_reconnect_flag == 0) || (memcmp(cur_bt_addr, bda, BT_BD_ADDR_LEN) != 0)) {
                yoc_app_bt_param_t param;
                memcpy(param.a2dp_conn.remote_addr, bda, BT_BD_ADDR_LEN);
                bt_callback(YOC_APP_BT_A2DP_CONNECTED, &param);
                if (g_bt_cb) {
                    g_bt_cb(YOC_APP_BT_A2DP_CONNECTED, &param);
                }
                g_a2dp_connect_state = YOC_A2D_CONNECTION_STATE_CONNECTED;
            }
            memcpy((void *)cur_bt_addr, (void *)bda, BT_BD_ADDR_LEN);
            bt_reconnect_flag = 0;
        } else if (a2d->conn_stat.state == YOC_A2D_CONNECTION_STATE_CONNECTING) {
            yoc_app_bt_param_t param;
            memcpy(param.a2dp_conn.remote_addr, bda, BT_BD_ADDR_LEN);
            bt_callback(YOC_APP_BT_A2DP_CONNECTING, &param);
            if (g_bt_cb) {
                g_bt_cb(YOC_APP_BT_A2DP_CONNECTING, &param);
            }
            g_a2dp_connect_state = YOC_A2D_CONNECTION_STATE_CONNECTING;
        }
        break;
    }
    case YOC_A2D_AUDIO_STATE_EVT: {

        LOGD(TAG, "A2DP audio state: %s", s_a2d_audio_state_str[a2d->audio_stat.state]);
        // g_tg_bt_audio_state = a2d->audio_stat.state;
        if (YOC_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
            avrcp_play_status_inquire_timer_start();

            bt_callback(YOC_APP_BT_A2DP_PLAY_STATUS_PLAYING, NULL);
            if (g_bt_cb) {
                g_bt_cb(YOC_APP_BT_A2DP_PLAY_STATUS_PLAYING, NULL);
            }

        } else if (YOC_A2D_AUDIO_STATE_REMOTE_SUSPEND == a2d->audio_stat.state ||
                   YOC_A2D_AUDIO_STATE_STOPPED == a2d->audio_stat.state) {
            avrcp_play_status_inquire_timer_stop();

            bt_callback(YOC_APP_BT_A2DP_PLAY_STATUS_STOPPED, NULL);
            if (g_bt_cb) {
                g_bt_cb(YOC_APP_BT_A2DP_PLAY_STATUS_STOPPED, NULL);
            }
        }

        break;
    }
    case YOC_A2D_AUDIO_CFG_EVT: {
        LOGD(TAG, "A2DP audio stream configuration, codec type %d", a2d->audio_cfg.mcc.type);
        // for now only SBC stream is supported
        if (a2d->audio_cfg.mcc.type == YOC_A2D_MCT_SBC) {
            LOGD(TAG, "Configure audio player %x-%x-%x-%x", a2d->audio_cfg.mcc.cie.sbc[0],
                 a2d->audio_cfg.mcc.cie.sbc[1], a2d->audio_cfg.mcc.cie.sbc[2],
                 a2d->audio_cfg.mcc.cie.sbc[3]);
        }
        break;
    }
    default:
        LOGE(TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

static void bt_av_new_track()
{
    //Register notifications and request metadata
    yoc_avrc_ct_send_metadata_cmd(_bt_get_lable(),
                                  YOC_AVRC_MD_ATTR_TITLE | YOC_AVRC_MD_ATTR_ARTIST |
                                      YOC_AVRC_MD_ATTR_ALBUM | YOC_AVRC_MD_ATTR_GENRE);
    yoc_avrc_ct_send_register_notification_cmd(_bt_get_lable(), YOC_AVRC_RN_TRACK_CHANGE, 0);
}

static void _bt_avrcp_notify_evt_handler(uint8_t event_id, yoc_avrc_rn_param_t *event_parameter)
{
    LOGE(TAG, "%s %d", __func__, event_id);
    yoc_app_bt_param_t param;

    switch (event_id) {
    case YOC_AVRC_RN_TRACK_CHANGE:
        bt_av_new_track();
        break;
    case YOC_AVRC_RN_VOLUME_CHANGE:
        /* FIXME */
        param.a2dp_vol.volume = event_parameter->volume;
        bt_callback(YOC_APP_BT_A2DP_VOLUME_CHANGE, &param);
        if (g_bt_cb) {
            g_bt_cb(YOC_APP_BT_A2DP_VOLUME_CHANGE, &param);
        }
        break;
    case YOC_AVRC_RN_PLAY_STATUS_CHANGE: {
        yoc_app_bt_event_t status;

        switch (event_parameter->playback) {
        case YOC_AVRC_PLAYBACK_STOPPED:
            status = YOC_APP_BT_AVRCP_STATUS_STOPPED;
            break;
        case YOC_AVRC_PLAYBACK_PLAYING:
            status = YOC_APP_BT_AVRCP_STATUS_PLAYING;
            break;
        case YOC_AVRC_PLAYBACK_PAUSED:
            status = YOC_APP_BT_AVRCP_STATUS_PAUSEED;
            break;
        default:
            return;
        }
        yoc_avrc_ct_send_register_notification_cmd(_bt_get_lable(), YOC_AVRC_RN_PLAY_STATUS_CHANGE,
                                                   0);
        bt_callback(status, NULL);
        if (g_bt_cb) {
            g_bt_cb(status, NULL);
        }
        break;
    }
    }
}

static void _bt_app_avrcp_ct_cb(yoc_avrc_ct_cb_event_t event, yoc_avrc_ct_cb_param_t *param)
{
    yoc_avrc_ct_cb_param_t *rc = (yoc_avrc_ct_cb_param_t *)(param);

    LOGD(TAG, "%s evt %d", __func__, event);
    switch (event) {
    case YOC_AVRC_CT_CONNECTION_STATE_EVT: {
        uint8_t *bda = rc->conn_stat.remote_bda;
        LOGD(TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
             rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        if (rc->conn_stat.connected) {
            bt_av_new_track();
            yoc_avrc_ct_send_register_notification_cmd(_bt_get_lable(),
                                                       YOC_AVRC_RN_PLAY_STATUS_CHANGE, 0);
        }
        break;
    }
    case YOC_AVRC_CT_PASSTHROUGH_RSP_EVT: {
        yoc_app_avrcp_cmd_type_t cmd;

        LOGD(TAG, "AVRC passthrough rsp: key_code 0x%x, key_state %d", rc->psth_rsp.key_code,
             rc->psth_rsp.key_state);
        switch (rc->psth_rsp.key_code) {
        case YOC_AVRC_PT_CMD_PLAY:
            cmd = YOC_APP_BT_AVRCP_CMD_PLAY;
            break;
        case YOC_AVRC_PT_CMD_PAUSE:
            cmd = YOC_APP_BT_AVRCP_CMD_PAUSE;
            break;
        case YOC_AVRC_PT_CMD_FORWARD:
            cmd = YOC_APP_BT_AVRCP_CMD_FORWARD;
            break;
        case YOC_AVRC_PT_CMD_BACKWARD:
            cmd = YOC_APP_BT_AVRCP_CMD_BACKWARD;
            break;
        case YOC_AVRC_PT_CMD_FAST_FORWARD:
            cmd = YOC_APP_BT_AVRCP_CMD_FAST_FORWARD;
            break;
        case YOC_AVRC_PT_CMD_REWIND:
            cmd = YOC_APP_BT_AVRCP_CMD_REWIND;
            break;
        case YOC_AVRC_PT_CMD_STOP:
            cmd = YOC_APP_BT_AVRCP_CMD_STOP;
            break;
        default:
            return;
        }

        yoc_app_bt_param_t param;
        param.a2dp_cmd.cmd = cmd;
        bt_callback(YOC_APP_BT_A2DP_CMD, &param);
        if (g_bt_cb) {
            g_bt_cb(YOC_APP_BT_A2DP_CMD, &param);
        }
        break;
    }
    case YOC_AVRC_CT_METADATA_RSP_EVT: {
        uint8_t *attr_text;

        attr_text = (uint8_t *)malloc(rc->meta_rsp.attr_length + 1);
        if (attr_text == NULL) {
            LOGD(TAG, "attr_text malloc failed");
            return;
        }
        memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
        attr_text[rc->meta_rsp.attr_length] = 0;

        LOGD(TAG, "AVRC metadata rsp: attribute id 0x%x, %s", rc->meta_rsp.attr_id, attr_text);
        free(attr_text);
        break;
    }
    case YOC_AVRC_CT_CHANGE_NOTIFY_EVT: {
        LOGD(TAG, "AVRC event notification: %d, param: %d", rc->change_ntf.event_id,
             rc->change_ntf.event_parameter);
        _bt_avrcp_notify_evt_handler(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
        break;
    }
    case YOC_AVRC_CT_REMOTE_FEATURES_EVT: {
        LOGD(TAG, "AVRC remote features %x", rc->rmt_feats.feat_mask);
        break;
    }
    case YOC_AVRC_CT_PLAY_STATUS_RSP_EVT: {
        LOGD(TAG, "AVRC get play status event");
        yoc_app_bt_param_t param;
        param.avrcp_get_play_status.play_status = rc->get_rn_play_status_rsp.play_status;
        param.avrcp_get_play_status.song_len    = rc->get_rn_play_status_rsp.song_len;
        param.avrcp_get_play_status.song_pos    = rc->get_rn_play_status_rsp.song_pos;

        bt_callback(YOC_APP_BT_AVRCP_GET_PLAY_STATUS, &param);
        if (g_bt_cb) {
            g_bt_cb(YOC_APP_BT_AVRCP_GET_PLAY_STATUS, &param);
        }
        break;
    }
    case YOC_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT: {
        LOGD(TAG, "AVRC VOLUME set %d", rc->set_volume_rsp.volume);
        yoc_app_bt_param_t param;
        param.a2dp_vol.volume = rc->set_volume_rsp.volume;
        bt_callback(YOC_APP_BT_A2DP_VOLUME_CHANGE, &param);
        if (g_bt_cb) {
            g_bt_cb(YOC_APP_BT_A2DP_VOLUME_CHANGE, &param);
        }
        break;
    }
    default:
        LOGE(TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

static void _bt_app_avrcp_tg_cb(yoc_avrc_tg_cb_event_t event, yoc_avrc_tg_cb_param_t *param)
{
    yoc_avrc_tg_cb_param_t *rc = (yoc_avrc_tg_cb_param_t *)(param);

    LOGD(TAG, "%s evt %d", __func__, event);
    switch (event) {
    case YOC_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
        if (rc->reg_ntf.event_id == YOC_AVRC_RN_VOLUME_CHANGE) {
            yoc_avrc_rn_param_t param;

            param.volume = 0x7f;
            yoc_avrc_tg_send_rn_rsp(YOC_AVRC_RN_VOLUME_CHANGE, YOC_AVRC_RN_RSP_INTERIM, &param);
        }
    } break;
    case YOC_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
        LOGD(TAG, "volume: %d", rc->set_abs_vol.volume);
        yoc_app_bt_param_t param;
        param.a2dp_vol.volume = rc->set_abs_vol.volume;
        bt_callback(YOC_APP_BT_A2DP_VOLUME_CHANGE, &param);
        if (g_bt_cb) {
            g_bt_cb(YOC_APP_BT_A2DP_VOLUME_CHANGE, &param);
        }
    } break;
    default:;
    }
}

static void _bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    static uint32_t s_pkt_cnt = 0;

    if (++s_pkt_cnt % 100 == 0) {
        LOGD(TAG, "Audio packet count %u", s_pkt_cnt);
    }
}

int yoc_app_bt_gap_set_scan_mode(int enable)
{
    bt_err_t ret;
    if (enable == 0) {
        ret = yoc_bt_gap_set_scan_mode(YOC_BT_NON_CONNECTABLE, YOC_BT_NON_DISCOVERABLE);
    } else {
        ret = yoc_bt_gap_set_scan_mode(YOC_BT_CONNECTABLE, YOC_BT_GENERAL_DISCOVERABLE);
    }

    LOGD(TAG, "set scan mode:%d ret:%d", enable, ret);
    return 0;
}

int yoc_app_bt_avrcp_send_passthrouth_cmd(yoc_app_avrcp_cmd_type_t cmd_type)
{
    int32_t cmd;

    LOGD(TAG, "%s start: %d", __func__, cmd_type);
    switch (cmd_type) {
    case YOC_APP_BT_AVRCP_CMD_PLAY:
        cmd = YOC_AVRC_PT_CMD_PLAY;
        break;
    case YOC_APP_BT_AVRCP_CMD_PAUSE:
        cmd = YOC_AVRC_PT_CMD_PAUSE;
        break;
    case YOC_APP_BT_AVRCP_CMD_FORWARD:
        cmd = YOC_AVRC_PT_CMD_FORWARD;
        break;
    case YOC_APP_BT_AVRCP_CMD_BACKWARD:
        cmd = YOC_AVRC_PT_CMD_BACKWARD;
        break;
    case YOC_APP_BT_AVRCP_CMD_FAST_FORWARD:
        cmd = YOC_AVRC_PT_CMD_FAST_FORWARD;
        break;
    case YOC_APP_BT_AVRCP_CMD_REWIND:
        cmd = YOC_AVRC_PT_CMD_REWIND;
        break;
    case YOC_APP_BT_AVRCP_CMD_STOP:
        cmd = YOC_AVRC_PT_CMD_STOP;
        break;
    case YOC_APP_BT_AVRCP_CMD_VOL_UP:
        cmd = YOC_AVRC_PT_CMD_VOL_UP;
        break;
    case YOC_APP_BT_AVRCP_CMD_VOL_DOWN:
        cmd = YOC_AVRC_PT_CMD_VOL_DOWN;
        break;
    default:
        return -1;
    }
    bt_err_t ret;

    aos_mutex_lock(&s_a2dp_mutex, AOS_WAIT_FOREVER);
    ret = yoc_avrc_ct_send_passthrough_cmd(_bt_get_lable(), cmd, 0);

    if (ret != BT_OK) {
        LOGE(TAG, "%s %d", __func__, 0);
    }

    ret = yoc_avrc_ct_send_passthrough_cmd(_bt_get_lable(), cmd, 1);

    if (ret != BT_OK) {
        LOGE(TAG, "%s %d", __func__, 1);
    }

    aos_mutex_unlock(&s_a2dp_mutex);

    return 0;
}

int yoc_app_bt_avrcp_change_vol(uint8_t vol)
{
    yoc_avrc_rn_param_t param;

    LOGD(TAG, "%s start: %d", __func__, vol);
    param.volume = vol;
    return yoc_avrc_tg_send_rn_rsp(YOC_AVRC_RN_VOLUME_CHANGE, YOC_AVRC_RN_RSP_CHANGED, &param);
}

void yoc_app_bt_avrcp_get_play_status(void)
{
    yoc_avrc_ct_send_get_play_status_cmd(_bt_get_lable());
}

int yoc_app_bt_a2dp_connect(uint8_t remote_addr[])
{
    return yoc_a2d_sink_connect(remote_addr);
}

int yoc_app_bt_a2dp_disconnect(void)
{
    int ret;

    ret = yoc_a2d_sink_disconnect(cur_bt_addr);
    memset((void *)cur_bt_addr, 0, BT_BD_ADDR_LEN); // 清除连接的MAC地址
    return ret;
}

int yoc_app_bt_a2dp_get_connect_status(void)
{
    return g_a2dp_connect_state;
}

unsigned char *yoc_app_bt_get_remote_addrss(void)
{
    return cur_bt_addr;
}

static int a2dp_audio_status;
int        yoc_app_bt_a2dp_get_status(void)
{
    return a2dp_audio_status;
}

int yoc_app_bt_set_device_name(char *name)
{
    int ret;
    ret = yoc_bt_dev_set_device_name(name);
    return ret;
}

int yoc_app_bt_a2dp_register_cb(yoc_app_bt_callback_t callback)
{
    g_bt_cb = callback;

    return 0;
}

static int bt_audio_state_app = YOC_AVRC_PLAYBACK_STOPPED;
void bt_audio_state_check(void)
{
    if((ctrl_bt_a2dp.status == SMTAUDIO_STATE_PLAYING) && ((bt_audio_state_app == YOC_AVRC_PLAYBACK_STOPPED) || (bt_audio_state_app == YOC_AVRC_PLAYBACK_PAUSED))) {
        //设备端处于播放状态, app处于非播放状态
        ctrl_bt_a2dp.status = SMTAUDIO_STATE_STOP;
        LOGD(TAG, "change a2dp status: %d", ctrl_bt_a2dp.status);
    } else if((bt_audio_state_app == YOC_AVRC_PLAYBACK_PLAYING) && ((ctrl_bt_a2dp.status == SMTAUDIO_STATE_STOP) || (ctrl_bt_a2dp.status == SMTAUDIO_STATE_PAUSE))) {
        //设备端处于非播放状态, app处于播放状态
        ctrl_bt_a2dp.status = SMTAUDIO_STATE_PLAYING;
        LOGD(TAG, "change a2dp status: %d", ctrl_bt_a2dp.status);
    }
}
static void bt_callback(yoc_app_bt_event_t event, yoc_app_bt_param_t *param)
{
    uint8_t *addr;

    int        smt_cur_state, smt_last_state;
    static int a2dp_tg_play_status;

    switch (event) {
    case YOC_APP_BT_PAIRED:
        addr = param->paired.remote_addr;
        LOGD(TAG, "PAIRED remote_addr: %02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2],
             addr[3], addr[4], addr[5]);
        break;
    case YOC_APP_BT_A2DP_CONNECTED:
        addr = param->a2dp_conn.remote_addr;
        LOGD(TAG, "CONNECTED remote_addr: %02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2],
             addr[3], addr[4], addr[5]);
        break;

    case YOC_APP_BT_A2DP_DISCONNECTED:
        addr = param->a2dp_conn.remote_addr;
        yoc_app_bt_gap_set_scan_mode(1);
        LOGD(TAG, "DISCONNECTED remote_addr: %02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1],
             addr[2], addr[3], addr[4], addr[5]);
        break;
    case YOC_APP_BT_A2DP_CONNECTING:
        addr = param->a2dp_conn.remote_addr;
        LOGD(TAG, "CONNECTING remote_addr: %02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1],
             addr[2], addr[3], addr[4], addr[5]);
        break;
    case YOC_APP_BT_A2DP_LINK_LOSS:
        addr = param->a2dp_conn.remote_addr;
        LOGD(TAG, "LINK_LOSS remote_addr: %02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2],
             addr[3], addr[4], addr[5]);
        break;
    case YOC_APP_BT_A2DP_PLAY_STATUS_STOPPED:
        LOGD(TAG, "YOC_APP_BT_A2DP_PLAY_STATUS_STOPPED");
        a2dp_audio_status = AUI_PLAYER_STOP;
        if (ctrl_bt_a2dp.callback) {
            ctrl_bt_a2dp.callback(SMTAUDIO_BT_A2DP, SMTAUDIO_PLAYER_EVENT_STOP);
        }
        break;
    case YOC_APP_BT_A2DP_PLAY_STATUS_PLAYING:
        LOGD(TAG, "YOC_APP_BT_A2DP_PLAY_STATUS_PLAYING");
        a2dp_audio_status = AUI_PLAYER_PLAYING;
        if (ctrl_bt_a2dp.callback) {
            ctrl_bt_a2dp.callback(SMTAUDIO_BT_A2DP, SMTAUDIO_PLAYER_EVENT_START);
        }
        break;
    case YOC_APP_BT_A2DP_PLAY_STATUS_PAUSEED:
        LOGD(TAG, "YOC_APP_BT_A2DP_PLAY_STATUS_PAUSEED");
        a2dp_audio_status = AUI_PLAYER_PAUSED;
        if (ctrl_bt_a2dp.callback) {
            ctrl_bt_a2dp.callback(SMTAUDIO_BT_A2DP, SMTAUDIO_PLAYER_EVENT_PAUSE);
        }
        break;
    case YOC_APP_BT_AVRCP_STATUS_PAUSEED:
        LOGD(TAG, "YOC_APP_BT_AVRCP_STATUS_PAUSEED");
        break;
    case YOC_APP_BT_AVRCP_STATUS_STOPPED:
        LOGD(TAG, "YOC_APP_BT_AVRCP_STATUS_STOPPED");
        break;
    case YOC_APP_BT_AVRCP_STATUS_PLAYING:
        LOGD(TAG, "YOC_APP_BT_AVRCP_STATUS_PLAYING");
        break;
    case YOC_APP_BT_AVRCP_GET_PLAY_STATUS:
        LOGD(TAG, "AVRC PLAY STATUS total_len:%d ms  cur:%d ms  play_status:%d",
             param->avrcp_get_play_status.song_len, param->avrcp_get_play_status.song_pos,
             param->avrcp_get_play_status.play_status);
        extern void smtaudio_substate_get(int *cur_state, int *last_state);
        smtaudio_substate_get(&smt_cur_state, &smt_last_state);
        a2dp_tg_play_status = param->avrcp_get_play_status.play_status;
        LOGD(TAG, "play status, tg:%d ct_cur:%d ct_last:%d", a2dp_tg_play_status, smt_cur_state,
             smt_last_state);
        static int bt_audio_state_app_last;
        bt_audio_state_app = param->avrcp_get_play_status.play_status;
        if(bt_audio_state_app == bt_audio_state_app_last) {
            bt_audio_state_check();
        }
        bt_audio_state_app_last = bt_audio_state_app;
        break;
    case YOC_APP_BT_A2DP_VOLUME_CHANGE:
        LOGD(TAG, "VOLUME_CHANGE: %d", param->a2dp_vol.volume);
        break;
    case YOC_APP_BT_A2DP_CMD:
        break;
    default:
        break;
    }
}

static void avrcp_play_status_inquire_timer_entry(void *timer, void *arg)
{
    yoc_app_bt_avrcp_get_play_status();
}
static void avrcp_play_status_inquire_timer_start(void)
{
    aos_timer_start(&s_a2dp_inquire_timer);
}
static void avrcp_play_status_inquire_timer_stop(void)
{
    aos_timer_stop(&s_a2dp_inquire_timer);
}

int yoc_app_bt_init()
{
    static int bt_init_flag;
    if (bt_init_flag) {
        return 0;
    }

    bt_init_flag = 1;

    bt_err_t err;
    err = yoc_bluedroid_init();
    if (err != BT_OK) {
        LOGE(TAG, "yoc_bluedroid_init err= %d", err);
        return -1;
    }

    err = yoc_bluedroid_enable();
    if (err != BT_OK) {
        LOGE(TAG, "yoc_bluedroid_enable err= %d", err);
        return -1;
    }

    err = yoc_bt_gap_register_callback(_bt_gap_cb);
    if (err) {
        LOGE(TAG, "gap register error, error code = %x", err);
        return -1;
    }
    return err;
}

static int yoc_app_bt_a2dp_init()
{
    static int a2dp_init_flag;
    if (a2dp_init_flag) {
        return 0;
    }
    a2dp_init_flag = 1;

    yoc_a2d_register_callback(_bt_app_a2d_cb);
    yoc_a2d_sink_register_data_callback(_bt_app_a2d_data_cb);
    yoc_a2d_sink_init();

    /* initialize AVRCP controller */
    yoc_avrc_ct_init();
    yoc_avrc_ct_register_callback(_bt_app_avrcp_ct_cb);

    yoc_avrc_tg_init();
    yoc_avrc_tg_register_callback(_bt_app_avrcp_tg_cb);

    aos_mutex_new(&s_a2dp_mutex);
    aos_timer_new_ext(&s_a2dp_inquire_timer, avrcp_play_status_inquire_timer_entry, NULL,
                      AVRCP_PLAY_STATUS_INQUIRE_TIME, 1, 0);

    // yoc_avrc_rn_evt_cap_mask_t evt_set;

    /* volume sync setup for IOS volume control */
    // evt_set.bits = 0x2000;
    // yoc_avrc_tg_set_rn_evt_cap(&evt_set);

    yoc_bt_io_cap_t bt_io_cap  = YOC_BT_IO_CAP_NONE;
    yoc_bt_io_cap_t ble_io_cap = YOC_BT_IO_CAP_NONE;

    yoc_bt_sp_param_t param_type_bt = YOC_BT_SP_IOCAP_MODE;
    yoc_bt_gap_set_security_param(param_type_bt, &bt_io_cap, sizeof(yoc_bt_io_cap_t));

    yoc_ble_sm_param_t param_type_ble = YOC_BLE_SM_IOCAP_MODE;
    yoc_ble_gap_set_security_param(param_type_ble, &ble_io_cap, sizeof(yoc_bt_io_cap_t));

    return 0;
}

static int bt_a2dp_init(void)
{
    yoc_app_bt_a2dp_init();
    return 0;
}

static int bt_a2dp_deinit(void)
{
    return 0;
}

static int bt_a2dp_start(const char *url, uint64_t seek_time, int resume)
{
    return yoc_app_bt_avrcp_send_passthrouth_cmd(YOC_APP_BT_AVRCP_CMD_PLAY);
}

static int bt_a2dp_pause(void)
{
    ctrl_bt_a2dp.status = SMTAUDIO_STATE_PAUSE;
    return yoc_app_bt_avrcp_send_passthrouth_cmd(YOC_APP_BT_AVRCP_CMD_PAUSE);
}

static int bt_a2dp_stop(void)
{
    ctrl_bt_a2dp.status = SMTAUDIO_STATE_STOP;
    return yoc_app_bt_avrcp_send_passthrouth_cmd(YOC_APP_BT_AVRCP_CMD_PAUSE);
}

static int bt_a2dp_resume(void)
{
    return yoc_app_bt_avrcp_send_passthrouth_cmd(YOC_APP_BT_AVRCP_CMD_PLAY);
}

static int bt_a2dp_vol_get(void)
{
    printf("%s invalid\n", __func__);
    return 0;
}

static int bt_a2dp_vol_set(int vol)
{
    return yoc_app_bt_avrcp_change_vol(vol * 127 / 100);
}

static int bt_a2dp_vol_up(int vol)
{
    int ret;
    /*调整 bt music 音量*/
    ret = yoc_app_bt_avrcp_send_passthrouth_cmd(YOC_APP_BT_AVRCP_CMD_VOL_UP);

    /*同时提高本地音音量*/
    smtaudio_ops_node_t *audio_default_ops;

    extern smtaudio_ops_node_t *get_default_audio_ops(void);
    audio_default_ops = get_default_audio_ops();
    if (audio_default_ops) {
        audio_default_ops->vol_up(vol);
    }

    return ret;
}

static int bt_a2dp_vol_down(int vol)
{
    int ret;
    /*调整 bt music 音量*/
    ret = yoc_app_bt_avrcp_send_passthrouth_cmd(YOC_APP_BT_AVRCP_CMD_VOL_DOWN);

    /*同时提高本地音音量*/
    smtaudio_ops_node_t *audio_default_ops;

    extern smtaudio_ops_node_t *get_default_audio_ops(void);
    audio_default_ops = get_default_audio_ops();
    if (audio_default_ops) {
        audio_default_ops->vol_down(vol);
    }

    return ret;
}

uint32_t smtaudio_register_bt_a2dp(uint8_t min_vol, uint8_t *aef_conf, size_t aef_conf_size, float speed, int resample)
{
    uint32_t id;
    yoc_a2d_audio_config_t config = {NULL, 0, 0};
    id = smtaudio_ops_register(&ctrl_bt_a2dp);
    ctrl_bt_a2dp.init();

    config.eqcfg = aef_conf;
    config.eqcfg_size = aef_conf_size;
        
    config.resample_rate = resample;
    yoc_a2d_sink_set_config(config);

    return id;
}
#endif
