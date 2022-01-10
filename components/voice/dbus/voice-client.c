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
#include <pthread.h>
#include <voice_debug.h>

#include <voice_hal.h>
#include <voice_client.h>

int method_call_get_retval_int32(DBusPendingCall *pending)
{
    int val;
    DBusMessage *msg;
    DBusMessageIter iter;

    dbus_pending_call_block(pending);

    msg = dbus_pending_call_steal_reply(pending);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Reply Null\n");
        exit(1);
    }

    dbus_pending_call_unref(pending);

    if (!dbus_message_iter_init(msg, &iter)) {
        voice_log(LOG_ERR, "Message has no arguments!\n");
    } else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&iter)) {
        voice_log(LOG_ERR, "Argument is not int32!\n");
    } else {
        dbus_message_iter_get_basic(&iter, &val);
    }

    dbus_message_unref(msg);

    return val;
}

int method_call_no_param_ret_int32(DBusConnection *conn, char *method)
{
    DBusMessage *msg;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(VOICE_DBUS_SERVER,
                                       VOICE_DBUS_PATH,
                                       VOICE_DBUS_INTERFACE,
                                       method);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        exit(1);
    }

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (NULL == pending) {
        voice_log(LOG_ERR, "Pending Call Null\n");
        exit(1);
    }

    dbus_connection_flush(conn);

    dbus_message_unref(msg);

    return method_call_get_retval_int32(pending);
}

int voice_client_func_start(voice_client_t *client)
{
    return method_call_no_param_ret_int32(client->conn, VOICE_DBUS_METHOD_CALL_START);
}

int voice_client_func_stop(voice_client_t *client)
{
    return method_call_no_param_ret_int32(client->conn, VOICE_DBUS_METHOD_CALL_STOP);
}

const char *voice_client_func_get_state(voice_client_t *client)
{
//    return (csi_kws_state_t)method_call_no_param_ret_int32(client->conn, VOICE_DBUS_METHOD_CALL_GET_STATE);

    DBusMessage *msg;
    DBusMessageIter iter;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(VOICE_DBUS_SERVER,
                                       VOICE_DBUS_PATH,
                                       VOICE_DBUS_INTERFACE,
                                       VOICE_DBUS_METHOD_CALL_GET_STATE);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        exit(1);
    }

    if (!dbus_connection_send_with_reply(client->conn, msg, &pending, -1)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (NULL == pending) {
        voice_log(LOG_ERR, "Pending Call Null\n");
        exit(1);
    }

    dbus_connection_flush(client->conn);

    dbus_message_unref(msg);


    char *str;

    dbus_pending_call_block(pending);

    msg = dbus_pending_call_steal_reply(pending);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Reply Null\n");
        exit(1);
    }

    dbus_pending_call_unref(pending);

    if (!dbus_message_iter_init(msg, &iter)) {
        voice_log(LOG_ERR, "Message has no arguments!\n");
    } else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&iter)) {
        voice_log(LOG_ERR, "Argument is not int32!\n");
    } else {
        dbus_message_iter_get_basic(&iter, &str);
    }

    dbus_message_unref(msg);

    return str;
}

int voice_client_func_pcm_config(voice_client_t *client, csi_pcm_format_t *format)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(VOICE_DBUS_SERVER,
                                       VOICE_DBUS_PATH,
                                       VOICE_DBUS_INTERFACE,
                                       VOICE_DBUS_METHOD_CALL_PCM_CONFIG);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        exit(1);
    }

    dbus_message_iter_init_append(msg, &iter);

    DBusMessageIter subiter;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, NULL, &subiter);

    if (!dbus_message_iter_append_basic(&subiter, DBUS_TYPE_INT32, &format->channel_num)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (!dbus_message_iter_append_basic(&subiter, DBUS_TYPE_INT32, &format->sample_rate)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (!dbus_message_iter_append_basic(&subiter, DBUS_TYPE_INT32, &format->sample_bits)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (!dbus_message_iter_append_basic(&subiter, DBUS_TYPE_INT32, &format->period_time)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    dbus_message_iter_close_container(&iter, &subiter);

    if (!dbus_connection_send_with_reply(client->conn, msg, &pending, -1)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (NULL == pending) {
        voice_log(LOG_ERR, "Pending Call Null\n");
        exit(1);
    }

    dbus_connection_flush(client->conn);

    dbus_message_unref(msg);

    return method_call_get_retval_int32(pending);
}

int voice_client_func_alg_config(voice_client_t *client, void *config, int size)
{
    return 0;
}

int voice_client_func_asr(voice_client_t *client, char *pcm, int size)
{
    return 0;
}

int voice_client_func_nlp(voice_client_t *client, const char *text)
{
    return 0;
}

int voice_client_func_tts(voice_client_t *client, const char *text)
{
    DBusMessage *msg;
    DBusMessageIter iter;
    DBusPendingCall *pending;
    char *str = (char *)text;

    msg = dbus_message_new_method_call(VOICE_DBUS_SERVER,
                                       VOICE_DBUS_PATH,
                                       VOICE_DBUS_INTERFACE,
                                       VOICE_DBUS_METHOD_CALL_TTS);

    if (NULL == msg) {
        voice_log(LOG_ERR, "Message Null\n");
        exit(1);
    }

    dbus_message_iter_init_append(msg, &iter);

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (!dbus_connection_send_with_reply(client->conn, msg, &pending, -1)) {
        voice_log(LOG_ERR, "Out Of Memory!\n");
        exit(1);
    }

    if (NULL == pending) {
        voice_log(LOG_ERR, "Pending Call Null\n");
        exit(1);
    }

    dbus_connection_flush(client->conn);

    dbus_message_unref(msg);

    return method_call_get_retval_int32(pending);
}

int get_keyword(DBusMessage *msg, void *kw)
{
    int ret = 0;
#if 0
    char *str;
    DBusMessageIter iter;
    DBusMessageIter subiter;

    if (!dbus_message_iter_init(msg, &iter)) {
        voice_log(LOG_ERR, "Message has no arguments!\n");
    }

    if (DBUS_TYPE_STRUCT != dbus_message_iter_get_arg_type(&iter)) {
        voice_log(LOG_ERR, "Argument is not struct!\n");
    }

    dbus_message_iter_recurse(&iter, &subiter);
    dbus_message_iter_get_basic(&subiter, &kw->id);
    dbus_message_iter_next(&subiter);
    dbus_message_iter_get_basic(&subiter, &kw->cred);
    dbus_message_iter_next(&subiter);
    dbus_message_iter_get_basic(&subiter, &str);

    strcpy((char *)&kw->word, str);
#endif

    return ret;
}

static void *client_thread_main(void *id)
{
    DBusError err;
    DBusMessage *msg[10];
    DBusConnection *conn;

    voice_client_t *client = (voice_client_t *)id;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

    if (conn == NULL) {
        exit(1);
    }

    client->conn = conn;

    if (dbus_error_is_set(&err)) {
        voice_log(LOG_ERR, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    dbus_bus_add_match(conn,
                       "type='signal',interface='org.voice.interface'",
                       &err);
    dbus_connection_flush(conn);

    if (dbus_error_is_set(&err)) {
        voice_log(LOG_ERR, "Match Error (%s)\n", err.message);
        exit(1);
    }

    while (true) {
        int count = 0;
        bool ret = dbus_connection_read_write(conn, 10);

        if (!ret) {
            usleep(10);
            continue;
        }

        for (int i = 0; i < 10; i++) {
            DBusMessage *tmp = dbus_connection_pop_message(conn);

            if (tmp) {
                msg[count++] = tmp;
            }
        }

        for (int i = 0; i < count; i++) {
            if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_SESSION_BEGIN)) {
//                csi_kws_keyword_t kw;
//                get_keyword(msg[i], &kw);
//                client->callback(VOICE_EVENT_SESSION_BEGIN, &kw, sizeof(kw));
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_SESSION_END)) {
                client->callback(VOICE_EVENT_SESSION_END, NULL, 0);
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_VAD)) {
                client->callback(VOICE_EVENT_VAD, NULL, 0);
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_ASR_BEGIN)) {
                client->callback(VOICE_EVENT_ASR_BEGIN, NULL, 0);
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_ASR_CHANGED)) {
                client->callback(VOICE_EVENT_ASR_CHANGED, NULL, 0);
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_ASR_END)) {
                client->callback(VOICE_EVENT_ASR_END, NULL, 0);
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_NLP)) {
                client->callback(VOICE_EVENT_NLP, NULL, 0);
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_TTS_BEGIN)) {
                client->callback(VOICE_EVENT_TTS_BEGIN, NULL, 0);
            } else if (dbus_message_is_signal(msg[i], VOICE_DBUS_INTERFACE, VOICE_DBUS_SIGNAL_TTS_END)) {
                client->callback(VOICE_EVENT_TTS_END, NULL, 0);
            }
        }
    }
}

void create_client_thread(voice_client_t *client)
{
    pthread_t pt;

    if (pthread_create(&pt, NULL, client_thread_main, client) != 0) {
        printf("thread create failed\n");
        return;
    }

    client->pthread = pt;
}

voice_client_t *voice_client_new(event_callback callback)
{
    voice_client_t *client = malloc(sizeof(voice_client_t));

    client->start      = (start_func)voice_client_func_start;
    client->stop       = (stop_func)voice_client_func_stop;
    client->get_state  = (get_state_func)voice_client_func_get_state;
    client->pcm_config = (pcm_config_func)voice_client_func_pcm_config;
    client->alg_config = (alg_config_func)voice_client_func_alg_config;
    client->asr        = (asr_func)voice_client_func_asr;
    client->nlp        = (nlp_func)voice_client_func_nlp;
    client->tts        = (tts_func)voice_client_func_tts;

    client->callback = (void *)callback;

    create_client_thread(client);

    usleep(1000000);
    //sem wait

    return client;
}

void voice_client_free(voice_client_t *client)
{
    pthread_cancel(client->pthread);

    free((void *)client);
}
#endif
