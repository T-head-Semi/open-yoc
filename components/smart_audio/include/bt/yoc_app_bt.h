/**
 * Copyright (C) 2019 Alibaba.inc, All rights reserved.
 */

#ifndef _YOC_APP_BT_H_
#define _YOC_APP_BT_H_

#ifndef BT_BD_ADDR_LEN
#define BT_BD_ADDR_LEN 6
#endif

#define YOC_APP_BDADDR_MAX_LEN (18)

typedef enum {
    YOC_APP_BT_AVRCP_CMD_PLAY = 0,
    YOC_APP_BT_AVRCP_CMD_PAUSE,
    YOC_APP_BT_AVRCP_CMD_FORWARD,
    YOC_APP_BT_AVRCP_CMD_BACKWARD,
    YOC_APP_BT_AVRCP_CMD_FAST_FORWARD,
    YOC_APP_BT_AVRCP_CMD_REWIND,
    YOC_APP_BT_AVRCP_CMD_STOP,
    YOC_APP_BT_AVRCP_CMD_VOL_UP,
    YOC_APP_BT_AVRCP_CMD_VOL_DOWN,
    YOC_APP_BT_AVRCP_CMD_MAX
} yoc_app_avrcp_cmd_type_t;

typedef enum {
    YOC_APP_BT_PAIRED,
    YOC_APP_BT_A2DP_CONNECTED,
    YOC_APP_BT_A2DP_DISCONNECTED,
    YOC_APP_BT_A2DP_CONNECTING,
    YOC_APP_BT_A2DP_LINK_LOSS,
    YOC_APP_BT_A2DP_PLAY_STATUS_STOPPED,
    YOC_APP_BT_A2DP_PLAY_STATUS_PLAYING,
    YOC_APP_BT_A2DP_PLAY_STATUS_PAUSEED,
    YOC_APP_BT_AVRCP_STATUS_PAUSEED,
    YOC_APP_BT_AVRCP_STATUS_STOPPED,
    YOC_APP_BT_AVRCP_STATUS_PLAYING,
    YOC_APP_BT_AVRCP_GET_PLAY_STATUS,
    YOC_APP_BT_A2DP_VOLUME_CHANGE,
    YOC_APP_BT_A2DP_CMD,
} yoc_app_bt_event_t;

typedef union yoc_app_bt_param {
    struct bt_paired_param {
        uint8_t remote_addr[BT_BD_ADDR_LEN];
    } paired;

    struct bt_a2dp_conn_param {
        uint8_t remote_addr[BT_BD_ADDR_LEN];
    } a2dp_conn;

    struct bt_a2dp_cmd_param {
        yoc_app_avrcp_cmd_type_t cmd;
    } a2dp_cmd;

    struct bt_a2dp_vol_change_param {
        uint8_t volume;
    } a2dp_vol;

    struct bt_avrcp_get_play_status_param {
        uint32_t song_len;
        uint32_t song_pos;
        uint8_t  play_status;
    } avrcp_get_play_status;
} yoc_app_bt_param_t;

typedef enum hfp_call_status {
    CALL_NONE = 0,
    CALL_NO_PROGRESS,
    CALL_IN_PROGRESS,
    CALL_INCOMING,
    CALL_OUTGOING_DIALING,
} yoc_app_bt_hfp_call_status_t;

typedef enum {
    YOC_APP_BT_HFP_CONNECTION_STATE_DISCONNECTED = 0, /*!< RFCOMM data link channel released */
    YOC_APP_BT_HFP_CONNECTION_STATE_CONNECTING, /*!< connecting remote device on the RFCOMM data link*/
    YOC_APP_BT_HFP_CONNECTION_STATE_CONNECTED,     /*!< RFCOMM connection established */
    YOC_APP_BT_HFP_CONNECTION_STATE_SLC_CONNECTED, /*!< service level connection established */
    YOC_APP_BT_HFP_CONNECTION_STATE_DISCONNECTING, /*!< disconnecting with remote device on the RFCOMM dat link*/
    YOC_APP_BT_HFP_CONNECTION_STATE_EVENT_MAX
} YOC_APP_BT_HFP_STATE_T;

typedef enum {
    YOC_APP_BT_HFP_AUDIO_STATE_DISCONNECTED = 0, /*!< audio connection released */
    YOC_APP_BT_HFP_AUDIO_STATE_CONNECTING,       /*!< audio connection has been initiated */
    YOC_APP_BT_HFP_AUDIO_STATE_CONNECTED,        /*!< audio connection is established */
    YOC_APP_BT_HFP_AUDIO_STATE_CONNECTED_MSBC,   /*!< mSBC audio connection is established */
    YOC_APP_BT_HFP_AUDIO_STATE_MAX
} YOC_APP_BT_HFP_AUDIO_STATE_T;

typedef enum {
    YOC_APP_BT_HFP_RING_TONE_IN_BAND = 0, /*!< in band ring tone */
    YOC_APP_BT_HFP_RING_TONE_NOT_IN_BAND, /*!< not in band ring tone */
    YOC_APP_BT_HFP_RING_TONE_TYPE_MAX,
} YOC_APP_BT_HFP_RING_TONE_T;

typedef enum {
/* Three-way calling */
    YOC_APP_BT_HFP_COMMAND_TYPE_REJECT_INCOMING = 0,              /* reject incoming call */
    YOC_APP_BT_HFP_COMMAND_TYPE_TERMINATE_ACTIVE_ACCEPT_INCOMING, /* terminate active and accept incoming */
    YOC_APP_BT_HFP_COMMAND_TYPE_HOLD_ACTIVE_ACCEPT_INCOMING, /* hold active and accept incoming */
/* Two-way calling */
    YOC_APP_BT_HFP_COMMAND_TYPE_ANSWER,                      /* answer call */
    YOC_APP_BT_HFP_COMMAND_TYPE_TERMINATE,                   /* terminate call */

    YOC_APP_BT_HFP_COMMAND_TYPE_MAX
} YOC_APP_BT_HFP_COMMAND_T;

typedef void (*yoc_app_bt_callback_t)(yoc_app_bt_event_t event, yoc_app_bt_param_t *param);

typedef void (*yoc_app_bt_hfp_state_changed_cb)(YOC_APP_BT_HFP_STATE_T state, const char *mac);
typedef void (*yoc_app_bt_hfp_audio_state_cb)(YOC_APP_BT_HFP_AUDIO_STATE_T state);
typedef void (*yoc_app_bt_hfp_volume_changed_cb)(uint8_t volume);
typedef void (*yoc_app_bt_hfp_ring_ind_cb)(YOC_APP_BT_HFP_RING_TONE_T type);

typedef struct {
    yoc_app_bt_hfp_state_changed_cb  hfpStateChangedCB;
    yoc_app_bt_hfp_audio_state_cb    hfpAudioStateCB;
    yoc_app_bt_hfp_volume_changed_cb hfpVolumeChangedCB;
    yoc_app_bt_hfp_ring_ind_cb       hfpRingIndCB;
} YOC_APP_BT_HFP_IMPL_CB_FUNC_T;

/**
 * @brief yoc_app_bt_gap_set_scan_mode
 * @param[in] 0 on disable or 1 on enable  
 * @return: 0 on success or -1 on error
 */
int yoc_app_bt_gap_set_scan_mode(int enable);

/**
 * @brief bt connect
 * @param[in] remote_addr: device mac address to connect
 * @return: BT_OK ot BT_FAIL
 */
int yoc_app_bt_a2dp_connect(uint8_t remote_addr[BT_BD_ADDR_LEN]);

/**
 * @brief break current connection
 * @return: BT_OK ot BT_FAIL
 */
int yoc_app_bt_a2dp_disconnect(void);

/**
 * @brief get current connection status
 * @return: connect status
 */
int yoc_app_bt_a2dp_get_connect_status(void);

/**
 * @brief get remote MAC address
 * @return: mac address
 */
unsigned char *yoc_app_bt_get_remote_addrss(void);

/**
 * @brief send avrvp conmmand
 * @param[in] cmd_type: avrcp command
 * @return: 0 on success or -1 on error
 */
int yoc_app_bt_avrcp_send_passthrouth_cmd(yoc_app_avrcp_cmd_type_t cmd_type);

/**
 * @brief change volume
 * @param[in] vol
 * @return: BT_OK ot BT_FAIL
 */
int yoc_app_bt_avrcp_change_vol(uint8_t vol);

/**
 * @brief get current avrcp play status 
 */
void yoc_app_bt_avrcp_get_play_status(void);

/**
 * @brief set bt name
 * @param[in] name
 * @return: BT_OK ot BT_FAIL
 */
int yoc_app_bt_set_device_name(char *name);

/**
 * @brief set bt callback for user
 * @param[in] callback
 */
int yoc_app_bt_a2dp_register_cb(yoc_app_bt_callback_t callback);

/**
 * @brief yoc bt init
 * @return: BT_OK ot BT_FAIL
 */
int yoc_app_bt_init(void);
/**
 * @brief Register hf callback
 * @param[in] callback: event callback set
 * @return: Zero on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_reg_callback(YOC_APP_BT_HFP_IMPL_CB_FUNC_T *callback);

/**
 * @brief connect hf
 * @param[in] mac: device mac address to connect
 * @return: 0 on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_connect(const char *mac);

/**
 * @brief Disconnect hfp
 * @param[in] mac: device mac address to disconnect
 * @return: Zero on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_disconnect(const char *mac);

/**
 * @brief Interrupt current connection
 * @return: Zero on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_interrupt_cur_conn(void);

/**
 * @brief connect hfp audio
 * @param[in] mac: device mac address to disconnect
 * @return: Zero on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_connect_audio(const char *mac);

/**
 * @brief Disconnect hfp audio
 * @param[in] mac: device mac address to disconnect
 * @return: Zero on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_disconnect_audio(const char *mac);

/**
 * @brief hfp send command
 * @param[in] command: command type
 * @return: Zero on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_send_command(YOC_APP_BT_HFP_COMMAND_T command);

/**
 * @brief hfp place a call
 * @param[in] number: number string of the call
 * @return: Zero on success or error code otherwise
 */
int32_t yoc_app_bt_hfp_dial(const char *number);

/**
 * @brief hfp get call status
 * @param[in]: None.
 * @return: the status of calling
 */
int32_t yoc_app_bt_hfp_get_call_status(void);

/**
 * @brief hfp set volume
 * @param[in]: type: 0-speaker, 1-microphone
 * @param[in]: volume: : volume gain ranges from 0 to 15
 */
int32_t yoc_app_bt_hfp_volume_update(int type, int volume);

#endif
