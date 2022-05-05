/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */
#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <voice.h>
#include <voice_hal.h>
#include <voice_debug.h>
#ifdef __linux__
#include "aos_port/common.h"
#endif

#include "dbus_knife/kdbus_utils.h"
#include "voice_server_dbus.h"

#define TAG                   "voice_ser"


static int should_dispatch;
struct kdbus_object_desc obj_dsc;

int voice_dbus_signal_session_begin(voice_t *voice, void *keyword, size_t size)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    char *kw = (void *)keyword;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_SESSION_BEGIN);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &kw)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    // dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &subiter);


    // dbus_message_iter_append_fixed_array(&subiter, DBUS_TYPE_BYTE, &kw, size);

    // dbus_message_iter_close_container(&iter, &subiter);

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_session_end(voice_t *voice)
{
    DBusMessage *msg;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_SESSION_END);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_shortcut_cmd(voice_t *voice, void *keyword, size_t size)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    void *kw = (void *)keyword;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_SHORTCUT_CMD);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &kw)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    // dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &subiter);

    // dbus_message_iter_append_fixed_array(&subiter, DBUS_TYPE_BYTE, &kw, size);

    // dbus_message_iter_close_container(&iter, &subiter);

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_network_err(voice_t *voice)
{
    DBusMessage *msg;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_NETWORK_ERR);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}


int voice_dbus_signal_vad(voice_t *voice)
{
    DBusMessage *msg;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_VAD);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_asr_begin(voice_t *voice)
{
    DBusMessage *msg;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_ASR_BEGIN);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_asr_changed(voice_t *voice, const char *text)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    char *str = (char *)text;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_ASR_CHANGED);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_asr_end(voice_t *voice, const char *text)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    char *str = (char *)text;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_ASR_END);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_tts_begin(voice_t *voice, const char *url)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    char tmp_url[200];
    memset(&tmp_url, 0, sizeof(tmp_url));
    strcpy((char *)&tmp_url, url);

    char *str = (char *)&tmp_url;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_TTS_BEGIN);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_tts_end(voice_t *voice, const char *url)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    char tmp_url[200];
    memset(&tmp_url, 0, sizeof(tmp_url));
    strcpy((char *)&tmp_url, url);

    char *str = (char *)&tmp_url;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_TTS_END);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}


int voice_dbus_signal_music_resp(voice_t *voice, const char *text)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    char *str = (char *)text;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_CALL_MUSIC_RESP);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

int voice_dbus_signal_mute_change(voice_t *voice, int new_state)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = voice->conn;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    msg = dbus_message_new_signal(VOICE_DBUS_PATH,
                                  VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_CALL_MUTE_CHANGE);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        return -1;
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &new_state)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, msg, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}


static int voice_dbus_method_start(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    csi_kws_t      *kws  = g_voice->kws;
    

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    ret_val = csi_kws_start(kws);

    if (voice_get_state(g_voice) == VOICE_STATE_STOPPED) {
        voice_set_state(g_voice, VOICE_STATE_IDLE);
    }

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_stop(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    csi_kws_t      *kws  = g_voice->kws;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    voice_set_state(g_voice, VOICE_STATE_STOPPED);

    ret_val = csi_kws_stop(kws);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_get_state(DBusConnection *conn, DBusMessage *msg)
{
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    char ret_str[10];
    char *str = (char *)&ret_str;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    voice_state_t state = voice_get_state(g_voice);

    memset(&ret_str, 0, sizeof(ret_str));

    if (state == VOICE_STATE_IDLE) {
        strcpy((char *)ret_str, "idle");
    } else if (state == VOICE_STATE_BUSY) {
        strcpy((char *)ret_str, "busy");
    } else {
        strcpy((char *)ret_str, "stopped");
    }

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_pcm_config(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    DBusMessageIter subiter;
    dbus_uint32_t serial = 0;
    csi_pcm_format_t format;
    csi_kws_t      *kws  = g_voice->kws;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    dbus_message_iter_init(msg, &iter);

    dbus_message_iter_recurse(&iter, &subiter);

    dbus_message_iter_get_basic(&subiter, &format.channel_num);

    if (!dbus_message_iter_next(&subiter)) {
        voice_log(LOG_ERR, "next failed!\n");
        return -1;
    }

    dbus_message_iter_get_basic(&subiter, &format.sample_rate);

    if (!dbus_message_iter_next(&subiter)) {
        voice_log(LOG_ERR, "next failed!\n");
        return -1;
    }

    dbus_message_iter_get_basic(&subiter, &format.sample_bits);

    if (!dbus_message_iter_next(&subiter)) {
        voice_log(LOG_ERR, "next failed!\n");
        return -1;
    }

    dbus_message_iter_get_basic(&subiter, &format.period_time);

    ret_val = csi_kws_pcm_config(kws, &format);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_alg_config(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    DBusMessageIter subiter;
    dbus_uint32_t serial = 0;
    csi_kws_t      *kws  = g_voice->kws;
    void *ptr;
    int n_elements;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    dbus_message_iter_init(msg, &iter);

    dbus_message_iter_recurse(&iter, &subiter);

    dbus_message_iter_get_fixed_array(&subiter, &ptr, &n_elements);

    ret_val = csi_kws_alg_config(kws, ptr, n_elements);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_asr(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_nlp(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    // extern void online_proc_post(void);
    // online_proc_post();

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_tts(DBusConnection *conn, DBusMessage *msg)
{
    char *str;
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    // aui_t          *aui  = g_voice->aui;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    dbus_message_iter_init(msg, &iter);
    int ret = dbus_message_iter_get_arg_type(&iter);

    if (ret != DBUS_TYPE_STRING) {
        printf("tts: not string\n");
        return -1;
    }

    dbus_message_iter_get_basic(&iter, &str);

    // aui_cloud_req_tts(aui, str);
    // extern int text_to_tts_parse(voice_t *voice, char *text);
    // text_to_tts_parse(voice, str);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}

static int voice_dbus_method_music_req(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    // extern int online_music_handle(void);
    // ret_val = online_music_handle();

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}


static int voice_dbus_method_mute_set(DBusConnection *conn, DBusMessage *msg)
{
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    int mute_state;

    dbus_message_iter_init(msg, &iter);
    int ret = dbus_message_iter_get_arg_type(&iter);

    if (ret != DBUS_TYPE_INT32) {
        printf("tts: not string\n");
        return -1;
    }

    dbus_message_iter_get_basic(&iter, &mute_state);

    voice_log(LOG_DEBUG, "Enter %s: mute status %d\n", __func__, mute_state);

    // extern int set_voice_mute_state(int state);
    // ret_val = set_voice_mute_state(mute_state);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}


static int voice_dbus_method_mute_get(DBusConnection *conn, DBusMessage *msg)
{
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    int mute_state;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    // extern int get_voice_mute_state(void);
    // mute_state = get_voice_mute_state();

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &mute_state)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}


static int voice_dbus_method_play(DBusConnection *conn, DBusMessage *msg)
{
    char *str;
    int ret_val;
    DBusMessage *reply;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;

    voice_log(LOG_DEBUG, "Enter %s\n", __func__);

    dbus_message_iter_init(msg, &iter);
    int ret = dbus_message_iter_get_arg_type(&iter);

    if (ret != DBUS_TYPE_STRING) {
        printf("play: not string\n");
        return -1;
    }

    dbus_message_iter_get_basic(&iter, &str);

    // char play_cmd[100];

    // memset(&play_cmd, 0, sizeof(play_cmd));
    // strncpy(&play_cmd[0], "aplay ", 6);
    // strcpy(&play_cmd[6], str);

    // ret_val = system((const char *)&play_cmd);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret_val)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send(conn, reply, &serial)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);

    dbus_message_unref(reply);

    voice_log(LOG_DEBUG, "Leave %s\n", __func__);

    return 0;
}



const struct kdbus_method_desc voice_dbus_global_methods[] = {
    {
        VOICE_DBUS_METHOD_CALL_START, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_start,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_STOP, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_stop,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_GET_STATE, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_get_state,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_PCM_CONFIG, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_pcm_config,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_ALG_CONFIG, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_alg_config,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_ASR, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_asr,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_NLP, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_nlp,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_TTS, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_tts,
        {
            { "args", "s", ARG_IN },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_PLAY, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_play,
        {
            { "args", "s", ARG_IN },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_MUSIC_REQ, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_music_req,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_MUTE_SET, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_mute_set,
        {
            { "args", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_METHOD_CALL_MUTE_GET, VOICE_DBUS_INTERFACE,
        (method_function) voice_dbus_method_mute_get,
        {
            { "args", "i", ARG_OUT },
            END_ARGS
        }
    },
    { NULL, NULL, NULL, { END_ARGS } }
};

static const struct kdbus_signal_desc voice_dbus_global_signals[] = {
    {
        VOICE_DBUS_SIGNAL_SESSION_BEGIN, VOICE_DBUS_INTERFACE,
        {
            // { "id", "d", ARG_OUT },
            // { "cred", "d", ARG_OUT },
            // { "word", "s", ARG_OUT },
            {"kw", "s", ARG_OUT},
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_SESSION_END, VOICE_DBUS_INTERFACE,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_SHORTCUT_CMD, VOICE_DBUS_INTERFACE,
        {
            {"kw", "s", ARG_OUT},
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_VAD, VOICE_DBUS_INTERFACE,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_NETWORK_ERR, VOICE_DBUS_INTERFACE,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_ASR_BEGIN, VOICE_DBUS_INTERFACE,
        {
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_ASR_CHANGED, VOICE_DBUS_INTERFACE,
        {
            { "text", "s", ARG_OUT },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_ASR_END, VOICE_DBUS_INTERFACE,
        {
            { "text", "s", ARG_OUT },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_NLP, VOICE_DBUS_INTERFACE,
        {
            { "text", "s", ARG_OUT },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_TTS_BEGIN, VOICE_DBUS_INTERFACE,
        {
            { "text", "s", ARG_OUT },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_TTS_END, VOICE_DBUS_INTERFACE,
        {
            { "text", "s", ARG_OUT },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_CALL_MUSIC_RESP, VOICE_DBUS_INTERFACE,
        {
            { "text", "s", ARG_OUT },
            END_ARGS
        }
    },
    {
        VOICE_DBUS_SIGNAL_CALL_MUTE_CHANGE, VOICE_DBUS_INTERFACE,
        {
            { "args", "i", ARG_OUT },
            END_ARGS
        }
    },
    { NULL, NULL, { END_ARGS } }

};



int voice_set_state(voice_t *voice, voice_state_t state)
{
    voice->state = state;

    return 0;
}

voice_state_t voice_get_state(voice_t *voice)
{
    return voice->state;
}


static void msg_method_handler(DBusMessage *msg, voice_t *voice)
{
    const char *member;

    voice_log(LOG_INFO, "Enter %s\n", __func__);

    member = dbus_message_get_member(msg);

    voice_log(LOG_INFO, "method call: %s\n", member);

    for (int i = 0; ; i++) {
        if (voice_dbus_global_methods[i].dbus_method == NULL) {
            voice_log(LOG_ERR, "not method call\n");
            break;
        }

        if (!strcmp(voice_dbus_global_methods[i].dbus_method, member)) {
            voice_dbus_global_methods[i].function(voice->conn, msg);
            break;
        }
    }
}

static DBusHandlerResult message_handler(DBusConnection *connection,
        DBusMessage *message, void *user_data)
{
    const char *method;
    const char *path;
    const char *interface;

    method = dbus_message_get_member(message);
    path = dbus_message_get_path(message);
    interface = dbus_message_get_interface(message);

    if (!method || !path || !interface) {
        return -1;
    }

    if (!strncmp(KDBUS_INTROSPECTION_METHOD, method, 50) &&
        !strncmp(KDBUS_INTROSPECTION_INTERFACE, interface, 50)) {
        kdbus_introspect(connection, message, &obj_dsc);
    } else if (!strncmp(VOICE_DBUS_INTERFACE, interface, 50)) {
        msg_method_handler(message, (voice_t *)user_data);
    } else {
        voice_log(LOG_ERR, "message error\n");
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

int voice_dbus_ctrl_iface_init(voice_t *voice)
{
    DBusObjectPathVTable vtable = {
        NULL, &message_handler,
        NULL, NULL, NULL, NULL
    };

    if (!dbus_connection_register_object_path(voice->conn, VOICE_DBUS_PATH, &vtable, voice)) {
        voice_log(LOG_ERR, "dbus_connection_register_object_path error\n");
        return -1;
    }

    return 0;
}

static dbus_bool_t add_watch(DBusWatch *watch, void *data)
{
    int fd;
    unsigned int flags;
    voice_t *voice = (voice_t *)data;

    if (!dbus_watch_get_enabled(watch)) {
        return TRUE;
    }

    flags = dbus_watch_get_flags(watch);
    fd = dbus_watch_get_unix_fd(watch);

    if (flags & DBUS_WATCH_READABLE) {
        voice->watch_rfd = fd;
        voice->watch = watch;
    }

    return TRUE;
}

static void wakeup_main(void *data)
{
    should_dispatch = 1;
}


static int integrate_dbus_watch(voice_t *voice)
{
    if (!dbus_connection_set_watch_functions(voice->conn, add_watch, NULL, NULL, voice, NULL)) {
        voice_log(LOG_ERR, "dbus_connection_set_watch_functions error\n");
        return -1;
    }

    dbus_connection_set_wakeup_main_function(voice->conn, wakeup_main,
            voice, NULL);


    return 0;
}

static void dispatch_data(DBusConnection *con)
{
    while (dbus_connection_get_dispatch_status(con) ==
           DBUS_DISPATCH_DATA_REMAINS) {
        dbus_connection_dispatch(con);
    }
}

static void process_watch(voice_t *voice, DBusWatch *watch)
{
    dbus_connection_ref(voice->conn);

    // should_dispatch = 0;

    dbus_watch_handle(watch, DBUS_WATCH_READABLE);

    if (should_dispatch) {
        dispatch_data(voice->conn);
        should_dispatch = 0;
    }

    dbus_connection_unref(voice->conn);
}

static void process_watch_read(voice_t *voice, DBusWatch *watch)
{
    process_watch(voice, watch);
}

// extern void set_record_config(void);
void voice_loop_run(voice_t *voice)
{
    int    sockfd;
    fd_set rset;
    int    retval;

    integrate_dbus_watch(voice);

    sockfd = voice->watch_rfd;

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);

    while (1) {
        retval = select(sockfd + 1, &rset, NULL, NULL, NULL);

        printf("voice_loop_run select retval %d (fd %d)\n", retval, sockfd);

        if (retval == -1) {
            voice_log(LOG_ERR, "select error\n");
        } else if (retval) {
            process_watch_read(voice, voice->watch);
        } else {
            voice_log(LOG_ERR, "select timeout\n");
            // timeout
        }

        // set_record_config();
    }
}


static void parse_msg(DBusMessage *msg, voice_t *v)
{
    const char *member, *path, *interface;

    member    = dbus_message_get_member(msg);
    path      = dbus_message_get_path(msg);
    interface = dbus_message_get_interface(msg);

    if (!member || !path || !interface) {
        printf("parse_msg error\n");
        return;
    }

    LOGI(TAG, "Get MSG, path: %s, inerf: %s, memb: %s", path, interface, member);

    if (!strcmp(member, "NameAcquired") && !strcmp(path, "/org/freedesktop/DBus") &&
        !strcmp(interface, "org.freedesktop.DBus")) {
        return;
    }

    if (!strcmp("Introspect", member) &&
        !strcmp("org.freedesktop.DBus.Introspectable", interface)) {
        kdbus_introspect(v->conn, msg, &obj_dsc);
        return;
    }

    for (int i = 0; ; i++) {
        if (voice_dbus_global_methods[i].dbus_method == NULL) {
            printf("not method call\n");
            break;
        }

        if (!strcmp(voice_dbus_global_methods[i].dbus_method, member)) {
            voice_dbus_global_methods[i].function(v->conn, msg);
            break;
        }
    }
}

void do_method_call_no_wait(voice_t *v)
{
    DBusMessage *msg;
    DBusConnection *conn = v->conn;

    dbus_connection_read_write_dispatch(conn, 0);
repop:
    msg = dbus_connection_pop_message(conn);
    if (msg) {
        parse_msg(msg, v);
        dbus_message_unref(msg);
        goto repop;
    }
}


void method_thread_main(void *arg)
{
    voice_t *v = (voice_t *)arg;

    while (1) {
        extern void do_method_call_no_wait(voice_t *v);
        do_method_call_no_wait(v);
        aos_msleep(20);
    }

    return;
}



int voice_dbus_init(voice_t *voice)
{
    int ret;
    DBusError err;
    DBusConnection *conn;

    dbus_threads_init_default();
    dbus_error_init(&err);

    conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

    voice->conn = conn;


    obj_dsc.methods = voice_dbus_global_methods;
    obj_dsc.signals = voice_dbus_global_signals;

    voice_dbus_ctrl_iface_init(voice);

    if (dbus_error_is_set(&err)) {
        voice_log(LOG_ERR, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if (conn == NULL) {
        exit(1);
    }

    ret = dbus_bus_request_name(conn, VOICE_DBUS_SERVER, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);

    if (dbus_error_is_set(&err)) {
        voice_log(LOG_ERR, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
	voice_log(LOG_ERR, "Error: Service has been started already\n");
        exit(1);
    }

    /////////////////////////////////////////////////////////////////////
    dbus_bus_add_match(conn,
                       "type='method_call',interface='org.voice.interface'",
                       &err);
    dbus_connection_flush(conn);

    if (dbus_error_is_set(&err)) {
        voice_log(LOG_ERR, "Match Error (%s)\n", err.message);
        exit(1);
    }

    return 0;
}

void voice_dbus_deinit(voice_t *voice)
{
    DBusError err;

    dbus_error_init(&err);

    dbus_bus_release_name(voice->conn, VOICE_DBUS_SERVER, &err);
}
#endif
