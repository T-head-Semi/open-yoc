/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AV_DBUS_SUPPORT) && CONFIG_AV_DBUS_SUPPORT
#include "dbus_knife/kdbus_utils.h"
#include "avdbus/dbus_utils.h"
#include "avdbus/media_client_dbus.h"
#include "media.h"

#define TAG "media_client"

static struct {
    int                       need_quit;
    DBusConnection            *conn;
    DBusConnection            *conn_listen;
    aos_task_t                media_cli_task;
    media_evt_t               evt_cb;
} g_media_client;

#define get_conn()            g_media_client.conn
#define get_conn_listen()     g_media_client.conn_listen

static int _kdbus_no_param_ret_int32(DBusConnection *conn, const char *method)
{
    int rc, val = -1;
    DBusMessage *msg;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(MEDIA_DBUS_SERVER,
                                       MEDIA_DBUS_PATH,
                                       MEDIA_DBUS_INTERFACE,
                                       method);
    CHECK_RET_TAG_WITH_RET(msg, -1);

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        LOGE(TAG, "send failed");
        goto err;
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    rc = kdbus_get_retval_int32(pending, &val);

    return rc == 0 ? -1 : val;
err:
    dbus_message_unref(msg);
    return -1;
}

static int _media_dbus_signal_play_event(DBusConnection *conn, DBusMessage *msg)
{
//    int type;
//    aui_player_evtid_t evt_id;
//    DBusMessageIter iter = {0}, subiter = {0};
//
//    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
//    dbus_message_iter_init(msg, &iter);
//    dbus_message_iter_recurse(&iter, &subiter);
//    dbus_message_iter_get_basic(&subiter, &type);
//    dbus_message_iter_next(&subiter);
//    dbus_message_iter_get_basic(&subiter, &evt_id);
//
//    g_media_client.evt_cb(type, evt_id);

    return 0;
}

static int _media_dbus_signal_play_error(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}
static int _media_dbus_signal_play_start(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_pause(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_resume(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_stop(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_finish(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_underrun(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_overrun(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_mute(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

static int _media_dbus_signal_play_vol_change(DBusConnection *conn, DBusMessage *msg)
{
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    //TODO:
    return 0;
}

typedef int (*dbus_handler_t)(DBusConnection *conn, DBusMessage *msg);
static struct {
    char            *name;
    dbus_handler_t  handler;
} g_signals[] = {
    { MEDIA_DBUS_SIGNAL_PLAY_ERROR, _media_dbus_signal_play_error },
    { MEDIA_DBUS_SIGNAL_PLAY_START, _media_dbus_signal_play_start },
    { MEDIA_DBUS_SIGNAL_PLAY_PAUSE, _media_dbus_signal_play_pause },
    { MEDIA_DBUS_SIGNAL_PLAY_RESUME, _media_dbus_signal_play_resume },
    { MEDIA_DBUS_SIGNAL_PLAY_STOP, _media_dbus_signal_play_stop },
    { MEDIA_DBUS_SIGNAL_PLAY_FINISH, _media_dbus_signal_play_finish },
    { MEDIA_DBUS_SIGNAL_PLAY_UNDER_RUN, _media_dbus_signal_play_underrun },
    { MEDIA_DBUS_SIGNAL_PLAY_OVER_RUN, _media_dbus_signal_play_overrun },
    { MEDIA_DBUS_SIGNAL_PLAY_MUTE, _media_dbus_signal_play_mute },
    { MEDIA_DBUS_SIGNAL_PLAY_VOL_CHANGE, _media_dbus_signal_play_vol_change }
};

static void _dispatch_msg(DBusConnection *conn, DBusMessage *msg)
{
    int rc, i;
#if 1
    const char *member, *path, *interface;

    member    = dbus_message_get_member(msg);
    path      = dbus_message_get_path(msg);
    interface = dbus_message_get_interface(msg);

    if (!member || !path || !interface) {
        LOGE(TAG, "parse_msg error");
        return;
    }
    LOGI(TAG, "Get MSG, path: %s, inerf: %s, memb: %s", path, interface, member);

    if (!strcmp(member, "NameAcquired") && !strcmp(path, "/org/freedesktop/DBus") &&
        !strcmp(interface, "org.freedesktop.DBus")) {
        return;
    }
    if (!strcmp("Introspect", member) &&
        !strcmp("org.freedesktop.DBus.Introspectable", interface)) {
        //TODO:
        //MEDIA_dbus_introspect(msg, &obj_dsc);
        return;
    }
#endif

    for (i = 0; i < ARRAY_SIZE(g_signals); i++) {
        if (dbus_message_is_signal(msg, MEDIA_DBUS_INTERFACE, g_signals[i].name)) {
            rc = g_signals[i].handler(conn, msg);
            LOGI(TAG, "signal call: name = %s, rc = %d", g_signals[i].name, rc);
            return;
        }
    }
    LOGI(TAG, "unknown signal call");
}

static void _media_cli_task(void *arg)
{
    DBusMessage *msg;
    DBusConnection *conn = get_conn_listen();

    while (!g_media_client.need_quit) {
        dbus_connection_read_write_dispatch(conn, -1);
repop:
        msg = dbus_connection_pop_message(conn);
        if (msg) {
            _dispatch_msg(conn, msg);
            dbus_message_unref(msg);
            goto repop;
        }
    }
}

/**
 * @brief  init the aui_player client
 * @param  [in] evt_cb
 * @return 0/-1
 */
int auic_player_init(media_evt_t evt_cb)
{
    int rc = -1;
    DBusError err;
    DBusConnection *conn = NULL, *conn_listen = NULL;

    CHECK_PARAM(evt_cb, -1);
    if (!g_media_client.conn) {
        dbus_error_init(&err);
        conn_listen = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);
        if (dbus_error_is_set(&err) || !conn_listen) {
            LOGE(TAG, "Connection Error (%s)", err.message);
            goto err;
        }
        conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);
        if (dbus_error_is_set(&err) || !conn) {
            LOGE(TAG, "Connection Error (%s)", err.message);
            goto err;
        }

        dbus_bus_add_match(conn_listen,
                           "type='signal',interface='"MEDIA_DBUS_INTERFACE"'",
                           &err);
        dbus_connection_flush(conn_listen);
        if (dbus_error_is_set(&err)) {
            LOGE(TAG, "Match Error (%s)", err.message);
            goto err;
        }
        g_media_client.conn        = conn;
        g_media_client.conn_listen = conn_listen;
        g_media_client.evt_cb      = evt_cb;

        rc = aos_task_new_ext(&g_media_client.media_cli_task, "media_cli_task", _media_cli_task, NULL, 40 * 1024, AOS_DEFAULT_APP_PRI);
        if (rc) {
            LOGE(TAG, "media client task creae fail, rc = %d", rc);
            goto err;
        }
        //rc = _kdbus_no_param_ret_int32(conn, "test");
        //printf("=======rc = %d\n", rc);
    }

    return 0;
err:
    if (conn) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
        g_media_client.conn = NULL;
    }
    if (conn_listen) {
        dbus_connection_close(conn_listen);
        dbus_connection_unref(conn_listen);
        g_media_client.conn_listen = NULL;
    }
    dbus_error_free(&err);
    return -1;
}

/**
 * @brief  play one url of the media-type
 * @param  [in] type : media type
 * @param  [in] url
 * @param  [in] conf
 * @return 0/-1
 */
int auic_player_play(int type, const char *url, auic_ply_t *conf)
{
    int rc = -1;
    int resume = 0, val = -1;
    uint64_t seek_time = 0;
    DBusMessage *msg;
    DBusPendingCall *pending = NULL;
    DBusConnection *conn = get_conn();
    DBusMessageIter iter = {0};
    DBusMessageIter iter_dict = {0};
    DBusMessageIter iter_dict_entry = {0}, iter_dict_val = {0};

    CHECK_PARAM(conn && url, -1);
    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    if (conf) {
        resume    = conf->resume;
        seek_time = conf->seek_time;
    }
    msg = dbus_message_new_method_call(MEDIA_DBUS_SERVER,
                                       MEDIA_DBUS_PATH,
                                       MEDIA_DBUS_INTERFACE,
                                       MEDIA_DBUS_METHOD_PLAY);
    CHECK_RET_TAG_WITH_RET(msg, -1);

    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(
        &iter,
        DBUS_TYPE_ARRAY,
        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
        DBUS_TYPE_STRING_AS_STRING
        DBUS_TYPE_VARIANT_AS_STRING
        DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
        &iter_dict);
    {
        char *key1 = "type";
        char *key2 = "url";
        char *key3 = "resume";
        char *key4 = "seek_time";

        dbus_message_iter_open_container(&iter_dict,
                                         DBUS_TYPE_DICT_ENTRY,
                                         NULL,
                                         &iter_dict_entry);
        dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key1);

        dbus_message_iter_open_container(&iter_dict_entry,
                                         DBUS_TYPE_VARIANT,
                                         DBUS_TYPE_INT32_AS_STRING,
                                         &iter_dict_val);
        dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_INT32, &type);
        dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
        dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);


        dbus_message_iter_open_container(&iter_dict,
                                         DBUS_TYPE_DICT_ENTRY,
                                         NULL,
                                         &iter_dict_entry);
        dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key2);

        dbus_message_iter_open_container(&iter_dict_entry,
                                         DBUS_TYPE_VARIANT,
                                         DBUS_TYPE_STRING_AS_STRING,
                                         &iter_dict_val);
        dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_STRING, &url);
        dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
        dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);

        dbus_message_iter_open_container(&iter_dict,
                                         DBUS_TYPE_DICT_ENTRY,
                                         NULL,
                                         &iter_dict_entry);
        dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key3);

        dbus_message_iter_open_container(&iter_dict_entry,
                                         DBUS_TYPE_VARIANT,
                                         DBUS_TYPE_INT32_AS_STRING,
                                         &iter_dict_val);
        dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_INT32, &resume);
        dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
        dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);

        dbus_message_iter_open_container(&iter_dict,
                                         DBUS_TYPE_DICT_ENTRY,
                                         NULL,
                                         &iter_dict_entry);
        dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key4);

        dbus_message_iter_open_container(&iter_dict_entry,
                                         DBUS_TYPE_VARIANT,
                                         DBUS_TYPE_UINT64_AS_STRING,
                                         &iter_dict_val);
        dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_UINT64, &seek_time);
        dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
        dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);
    }
    dbus_message_iter_close_container(&iter, &iter_dict);

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        LOGE(TAG, "send failed");
        goto err;
    }
    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    rc = kdbus_get_retval_int32(pending, &val);

    return rc != 0 ? -1 : val;
err:
    dbus_message_unref(msg);
    return -1;
}

static int _do_method_call1(DBusConnection *conn, const char *method, int type)
{
    int rc, val = -1;
    DBusMessage *msg;
    DBusMessageIter iter = {0};
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(MEDIA_DBUS_SERVER,
                                       MEDIA_DBUS_PATH,
                                       MEDIA_DBUS_INTERFACE,
                                       method);
    CHECK_RET_TAG_WITH_RET(msg, -1);

    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &type);
    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        LOGE(TAG, "send failed");
        goto err;
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    rc = kdbus_get_retval_int32(pending, &val);

    return rc != 0 ? -1 : val;
err:
    dbus_message_unref(msg);
    return -1;
}

static int _do_method_call2(DBusConnection *conn, const char *method, int type, int value)
{
    int rc, val = -1;
    DBusMessage *msg;
    DBusPendingCall *pending;
    DBusMessageIter iter = {0}, subiter = {0};

    msg = dbus_message_new_method_call(MEDIA_DBUS_SERVER,
                                       MEDIA_DBUS_PATH,
                                       MEDIA_DBUS_INTERFACE,
                                       method);
    CHECK_RET_TAG_WITH_RET(msg, -1);
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, NULL, &subiter);
    dbus_message_iter_append_basic(&subiter, DBUS_TYPE_INT32, &type);
    dbus_message_iter_append_basic(&subiter, DBUS_TYPE_INT32, &value);
    dbus_message_iter_close_container(&iter, &subiter);

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        LOGE(TAG, "send failed");
        goto err;
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    rc = kdbus_get_retval_int32(pending, &val);

    return rc == 0 ? -1 : val;
err:
    dbus_message_unref(msg);
    return -1;
}

/**
* @brief  pause play the media type
 * @param  [in] type : media type
* @return 0/-1
*/
int auic_player_pause(int type)
{
    int rc = -1;
    DBusConnection *conn = get_conn();

    rc = _do_method_call1(conn, MEDIA_DBUS_METHOD_PAUSE, type);
    LOGI(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return rc;
}

/**
* @brief  resume play the media type
 * @param  [in] type : media type
* @return 0/-1
*/
int auic_player_resume(int type)
{
    int rc = -1;
    DBusConnection *conn = get_conn();

    rc = _do_method_call1(conn, MEDIA_DBUS_METHOD_RESUME, type);
    LOGI(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return rc;
}

/**
* @brief  stop play the media type
 * @param  [in] type : media type
* @return 0/-1
*/
int auic_player_stop(int type)
{
    int rc = -1;
    DBusConnection *conn = get_conn();

    rc = _do_method_call1(conn, MEDIA_DBUS_METHOD_STOP, type);
    LOGI(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return rc;
}

/**
* @brief  mute play the media type
 * @param  [in] type : media type
* @return 0/-1
*/
int auic_player_mute(int type)
{
    int rc = -1;
    DBusConnection *conn = get_conn();

    rc = _do_method_call1(conn, MEDIA_DBUS_METHOD_MUTE, type);
    LOGI(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return rc;
}

/**
* @brief  unmute play the media type
 * @param  [in] type : media type
* @return 0/-1
*/
int auic_player_unmute(int type)
{
    int rc = -1;
    DBusConnection *conn = get_conn();

    rc = _do_method_call1(conn, MEDIA_DBUS_METHOD_UNMUTE, type);
    LOGI(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return rc;
}

/**
* @brief  seek play the media type
 * @param  [in] type : media type
 * @param  [in] seek_time : ms
* @return 0/-1
 */
int auic_player_seek(int type, uint64_t seek_time)
{
    int rc = -1, val = -1;
    DBusMessage *msg;
    DBusPendingCall *pending = NULL;
    DBusConnection *conn = get_conn();
    DBusMessageIter iter = {0}, subiter = {0};

    msg = dbus_message_new_method_call(MEDIA_DBUS_SERVER,
                                       MEDIA_DBUS_PATH,
                                       MEDIA_DBUS_INTERFACE,
                                       MEDIA_DBUS_METHOD_SEEK);
    CHECK_RET_TAG_WITH_RET(msg, -1);

    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, NULL, &subiter);
    dbus_message_iter_append_basic(&subiter, DBUS_TYPE_INT32, &type);
    dbus_message_iter_append_basic(&subiter, DBUS_TYPE_UINT64, &seek_time);
    dbus_message_iter_close_container(&iter, &subiter);

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        LOGE(TAG, "send failed");
        goto err;
    }
    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    rc = kdbus_get_retval_int32(pending, &val);
    LOGI(TAG, "%s, %d, rc = %d, val = %d", __FUNCTION__, __LINE__, rc, val);

    return rc != 0 ? -1 : val;
err:
    dbus_message_unref(msg);
    return -1;
}
#endif



