/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AV_DBUS_SUPPORT) && CONFIG_AV_DBUS_SUPPORT
#include "avformat/avformat_utils.h"
#include "dbus_knife/kdbus_utils.h"
#include "avdbus/dbus_utils.h"
#include "avdbus/media_server_dbus.h"
#include "dbus_knife/kdbus_introspect.h"
#include "media.h"

#define TAG                   "media_ser"

static struct kdbus_object_desc g_obj_dsc;

static struct {
    int                       need_quit;
    int                       start;
    DBusConnection            *conn;
    DBusConnection            *conn_listen;
    aos_task_t                media_ser_task;
    aos_mutex_t               lock;
} g_media_server;

#define get_conn()            g_media_server.conn
#define get_conn_listen()     g_media_server.conn_listen

#define media_srv_lock()   (aos_mutex_lock(&g_media_server.lock, AOS_WAIT_FOREVER))
#define media_srv_unlock() (aos_mutex_unlock(&g_media_server.lock))

static int _media_dbus_send_play_event(int type, const char *url, aui_player_evtid_t evt_id, const void *data, size_t size)
{
    int rc = -1;
    char *name;
    char *key1 = "type";
    char *key2 = "url";
    DBusMessage *msg;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = get_conn();
    DBusMessageIter iter = {0};
    DBusMessageIter iter_dict = {0}, variant_iter = {0};
    DBusMessageIter iter_dict_entry = {0}, iter_dict_val = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    name = (char*)get_name_by_evtid(evt_id);
    if (!name) {
        LOGD(TAG, "evt not handle, %d", evt_id);
        return -1;
    }
    msg = dbus_message_new_signal(MEDIA_DBUS_PATH,
                                  MEDIA_DBUS_INTERFACE, name);
    CHECK_RET_TAG_WITH_RET(msg, -1);

    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "a{sv}", &variant_iter);
    dbus_message_iter_open_container(
        &variant_iter,
        DBUS_TYPE_ARRAY,
        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
        DBUS_TYPE_STRING_AS_STRING
        DBUS_TYPE_VARIANT_AS_STRING
        DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
        &iter_dict);


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

    switch (evt_id) {
    case AUI_PLAYER_EVENT_MUTE: {
        char *key3 = "mute";
        int *mute = (int*)data;

        dbus_message_iter_open_container(&iter_dict,
                                         DBUS_TYPE_DICT_ENTRY,
                                         NULL,
                                         &iter_dict_entry);
        dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key3);

        dbus_message_iter_open_container(&iter_dict_entry,
                                         DBUS_TYPE_VARIANT,
                                         DBUS_TYPE_INT32_AS_STRING,
                                         &iter_dict_val);
        dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_INT32, mute);
        dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
        dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);
        break;
    }
    case AUI_PLAYER_EVENT_VOL_CHANGE: {
        char *key3 = "valOld";
        char *key4 = "valNew";

        aui_val_change_t *vc = (aui_val_change_t*)data;

        dbus_message_iter_open_container(&iter_dict,
                                         DBUS_TYPE_DICT_ENTRY,
                                         NULL,
                                         &iter_dict_entry);
        dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key3);

        dbus_message_iter_open_container(&iter_dict_entry,
                                         DBUS_TYPE_VARIANT,
                                         DBUS_TYPE_INT32_AS_STRING,
                                         &iter_dict_val);
        dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_INT32, &vc->val_old);
        dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
        dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);

        dbus_message_iter_open_container(&iter_dict,
                                         DBUS_TYPE_DICT_ENTRY,
                                         NULL,
                                         &iter_dict_entry);
        dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key4);

        dbus_message_iter_open_container(&iter_dict_entry,
                                         DBUS_TYPE_VARIANT,
                                         DBUS_TYPE_INT32_AS_STRING,
                                         &iter_dict_val);
        dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_INT32, &vc->val_new);
        dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
        dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);
        break;
    }
    default:
        break;
    }
    dbus_message_iter_close_container(&variant_iter, &iter_dict);
    dbus_message_iter_close_container(&iter, &variant_iter);

    if (!dbus_connection_send(conn, msg, &serial)) {
        LOGE(TAG, "send failed");
        goto err;
    }
    dbus_connection_flush(conn);
    rc = 0;

err:
    dbus_message_unref(msg);
    return rc;
}

static void _media_evt(int type, const char *url, aui_player_evtid_t evt_id, const void *data, size_t size)
{
    LOGD(TAG, "media_evt type %d,evt_id %d, url = %s", type, evt_id, url ? url : "default");

    //FIXME:
    _media_dbus_send_play_event(type, url, evt_id, data, size);
    return;
}

/**
 * _dbus_error_invalid_args - Return a new InvalidArgs error message
 * @message: Pointer to incoming dbus message this error refers to
 * Returns: a dbus error message
 *
 * Convenience function to create and return an invalid options error
 */
DBusMessage * _dbus_error_invalid_args(DBusMessage *message,
                                       const char *arg)
{
#if 0
    DBusMessage *reply;

    reply = dbus_message_new_error(
                message, WPAS_DBUS_ERROR_INVALID_ARGS,
                "Did not receive correct message arguments.");
    if (arg != NULL)
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &arg,
                                 DBUS_TYPE_INVALID);

    return reply;
#else
    //TODO:
    return NULL;
#endif
}

static int __media_dbus_get_var_param0(DBusMessage *msg, int *ptype, int *pval)
{
    int flag = 0;
    char *key = NULL;
    int type = 0, val = 0;
    DBusMessageIter iter = {0}, dict_iter = {0}, entry_iter = {0}, variant_iter = {0};

    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_recurse(&iter, &dict_iter);
    while (dbus_message_iter_get_arg_type(&dict_iter) ==
           DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &key);
        dbus_message_iter_next(&entry_iter);
        dbus_message_iter_recurse(&entry_iter, &variant_iter);

        if (strcmp(key, "type") == 0) {
            flag |= 0x1;
            dbus_message_iter_get_basic(&variant_iter, &type);
        } else if (strcmp(key, "value") == 0) {
            flag |= 0x10;
            dbus_message_iter_get_basic(&variant_iter, &val);
        } else {
            return -1;
        }

        dbus_message_iter_next(&dict_iter);
    }

    if (flag != 0x11) {
        LOGE(TAG, "method param fail, flag = 0x%x", flag);
        return -1;
    }
    *ptype = type;
    *pval  = val;

    return 0;
}

static int __media_dbus_get_var_param1(DBusMessage *msg, int *ptype, xwindow_t *pwin)
{
    int flag = 0;
    char *key = NULL;
    int type = 0, x = 0, y = 0, width = 0, height = 0;
    DBusMessageIter iter = {0}, dict_iter = {0}, entry_iter = {0}, variant_iter = {0};

    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_recurse(&iter, &dict_iter);
    while (dbus_message_iter_get_arg_type(&dict_iter) ==
           DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &key);
        dbus_message_iter_next(&entry_iter);
        dbus_message_iter_recurse(&entry_iter, &variant_iter);

        if (strcmp(key, "type") == 0) {
            flag |= 0x1;
            dbus_message_iter_get_basic(&variant_iter, &type);
        } else if (strcmp(key, "x") == 0) {
            flag |= 0x10;
            dbus_message_iter_get_basic(&variant_iter, &x);
        } else if (strcmp(key, "y") == 0) {
            flag |= 0x100;
            dbus_message_iter_get_basic(&variant_iter, &y);
        } else if (strcmp(key, "width") == 0) {
            flag |= 0x1000;
            dbus_message_iter_get_basic(&variant_iter, &width);
        } else if (strcmp(key, "height") == 0) {
            flag |= 0x10000;
            dbus_message_iter_get_basic(&variant_iter, &height);
        } else {
            return -1;
        }

        dbus_message_iter_next(&dict_iter);
    }

    if (flag != 0x11111) {
        LOGE(TAG, "method param fail, flag = 0x%x", flag);
        return -1;
    }
    *ptype       = type;
    pwin->x      = x;
    pwin->y      = y;
    pwin->width  = width;
    pwin->height = height;

    return 0;
}

static int _media_dbus_method_play(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    char *url = NULL;
    char *key = NULL;
    int type = 0, resume = 0;
    uint64_t seek_time = 0;
    DBusMessageIter iter = {0}, dict_iter = {0}, entry_iter = {0}, variant_iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_recurse(&iter, &dict_iter);
    while (dbus_message_iter_get_arg_type(&dict_iter) ==
           DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &key);
        dbus_message_iter_next(&entry_iter);
        dbus_message_iter_recurse(&entry_iter, &variant_iter);

        if (strcmp(key, "type") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &type);
        } else if (strcmp(key, "url") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &url);
        } else if (strcmp(key, "resume") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &resume);
        } else if (strcmp(key, "seek_time") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &seek_time);
        } else {
            return -1;
        }

        dbus_message_iter_next(&dict_iter);
    }

    rc = aui_player_seek_play(type, url, seek_time, resume);
    LOGD(TAG, "%s, %d, rc = %d, type = %d, seek_time = %llu, resume = %d, url = %s", __FUNCTION__, __LINE__, rc, type,
         seek_time, resume, url);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_pause(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_pause(type);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_resume(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_resume(type);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_stop(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    LOGI(TAG, "%s, %d enter, arg type = '%c'", __FUNCTION__, __LINE__, dbus_message_iter_get_arg_type(&iter));
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_stop(type);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_mute(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_mute(type);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_unmute(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_unmute(type);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_get_vol(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    LOGI(TAG, "%s, %d enter, arg type = '%c'", __FUNCTION__, __LINE__, dbus_message_iter_get_arg_type(&iter));
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_vol_get(type);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_vol(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_vol_set(type, val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_seek(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0;
    char *key = NULL;
    uint64_t seek_time = 0;
    DBusMessageIter iter = {0}, dict_iter = {0}, entry_iter = {0}, variant_iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_recurse(&iter, &dict_iter);
    while (dbus_message_iter_get_arg_type(&dict_iter) ==
           DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &key);
        dbus_message_iter_next(&entry_iter);
        dbus_message_iter_recurse(&entry_iter, &variant_iter);

        if (strcmp(key, "type") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &type);
        } else if (strcmp(key, "value") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &seek_time);
        } else {
            return -1;
        }

        dbus_message_iter_next(&dict_iter);
    }

    rc = aui_player_seek(type, seek_time);
    LOGD(TAG, "%s, %d, rc = %d, type = %d, seek_time = %llu", __FUNCTION__, __LINE__, rc, type, seek_time);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_speed(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 10;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_set_speed(type, (float)val / 10);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_get_speed(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessageIter iter = {0};
    float speed = 1.0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_get_speed(type, &speed);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    return kdbus_set_retval_int32(conn, msg, (int)(speed * 10));
}

static int _media_dbus_method_get_status(DBusConnection *conn, DBusMessage *msg)
{
    int type;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    aui_player_state_t status = aui_player_get_state(type);
    LOGD(TAG, "%s, %d, status = %d", __FUNCTION__, __LINE__, status);

    return kdbus_set_retval_int32(conn, msg, (int)status);
}

static int _media_dbus_method_get_media_info(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    DBusMessage *reply = NULL;
    dbus_uint32_t serial = 0;
    xmedia_info_t minfo = {0};
    DBusMessageIter iter = {0};
    DBusMessageIter iter_dict = {0}, variant_iter = {0};
    DBusMessageIter iter_dict_entry = {0}, iter_dict_val = {0};
    char *key1 = "size", *key2 = "bps", *key3 = "duration";

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    //FIXME: no track info now
    media_info_init(&minfo);
    rc = aui_player_get_media_info(type, &minfo);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "a{sv}", &variant_iter);

    dbus_message_iter_open_container(
        &variant_iter,
        DBUS_TYPE_ARRAY,
        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
        DBUS_TYPE_STRING_AS_STRING
        DBUS_TYPE_VARIANT_AS_STRING
        DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
        &iter_dict);

    dbus_message_iter_open_container(&iter_dict,
                                     DBUS_TYPE_DICT_ENTRY,
                                     NULL,
                                     &iter_dict_entry);
    dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key1);

    dbus_message_iter_open_container(&iter_dict_entry,
                                     DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_UINT64_AS_STRING,
                                     &iter_dict_val);
    dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_UINT64, &minfo.size);
    dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
    dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);


    dbus_message_iter_open_container(&iter_dict,
                                     DBUS_TYPE_DICT_ENTRY,
                                     NULL,
                                     &iter_dict_entry);
    dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key2);

    dbus_message_iter_open_container(&iter_dict_entry,
                                     DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_UINT64_AS_STRING,
                                     &iter_dict_val);
    dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_UINT64, &minfo.bps);
    dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
    dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);

    dbus_message_iter_open_container(&iter_dict,
                                     DBUS_TYPE_DICT_ENTRY,
                                     NULL,
                                     &iter_dict_entry);
    dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key3);

    dbus_message_iter_open_container(&iter_dict_entry,
                                     DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_UINT64_AS_STRING,
                                     &iter_dict_val);
    dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_UINT64, &minfo.duration);
    dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
    dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);

    dbus_message_iter_close_container(&variant_iter, &iter_dict);
    dbus_message_iter_close_container(&iter, &variant_iter);

    if (!dbus_connection_send(conn, reply, &serial)) {
        rc = -1;
        LOGE(TAG, "send failed");
        goto err;
    }
    dbus_connection_flush(conn);
    rc = 0;

err:
    dbus_message_unref(reply);
    media_info_uninit(&minfo);
    return rc;
}

static int _media_dbus_method_get_time(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type;
    char *key1 = "duration";
    char *key2 = "curtime";
    DBusMessage *reply = NULL;
    dbus_uint32_t serial = 0;
    aui_play_time_t time = {0};
    DBusMessageIter iter = {0};
    DBusMessageIter iter_dict = {0}, variant_iter = {0};
    DBusMessageIter iter_dict_entry = {0}, iter_dict_val = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &type);

    rc = aui_player_get_time(type, &time);
    LOGD(TAG, "%s, %d, rc = %d", __FUNCTION__, __LINE__, rc);

    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "a{sv}", &variant_iter);

    dbus_message_iter_open_container(
        &variant_iter,
        DBUS_TYPE_ARRAY,
        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
        DBUS_TYPE_STRING_AS_STRING
        DBUS_TYPE_VARIANT_AS_STRING
        DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
        &iter_dict);

    dbus_message_iter_open_container(&iter_dict,
                                     DBUS_TYPE_DICT_ENTRY,
                                     NULL,
                                     &iter_dict_entry);
    dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key1);

    dbus_message_iter_open_container(&iter_dict_entry,
                                     DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_UINT64_AS_STRING,
                                     &iter_dict_val);
    dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_UINT64, &time.duration);
    dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
    dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);


    dbus_message_iter_open_container(&iter_dict,
                                     DBUS_TYPE_DICT_ENTRY,
                                     NULL,
                                     &iter_dict_entry);
    dbus_message_iter_append_basic(&iter_dict_entry, DBUS_TYPE_STRING, &key2);

    dbus_message_iter_open_container(&iter_dict_entry,
                                     DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_UINT64_AS_STRING,
                                     &iter_dict_val);
    dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_UINT64, &time.curtime);
    dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
    dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);


    dbus_message_iter_close_container(&variant_iter, &iter_dict);
    dbus_message_iter_close_container(&iter, &variant_iter);

    if (!dbus_connection_send(conn, reply, &serial)) {
        rc = -1;
        LOGE(TAG, "send failed");
        goto err;
    }
    dbus_connection_flush(conn);
    rc = 0;

err:
    dbus_message_unref(reply);
    return rc;
}

#if !CONFIG_AV_AUDIO_ONLY_SUPPORT
static int _media_dbus_method_switch_audio_track(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_switch_audio_track(type, (uint8_t)val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_switch_subtitle_track(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_switch_subtitle_track(type, (uint8_t)val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_subtitle_url(DBusConnection *conn, DBusMessage *msg)
{
    int rc, type = 0;
    char *key = NULL, *url = NULL;
    DBusMessageIter iter = {0}, dict_iter = {0}, entry_iter = {0}, variant_iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_recurse(&iter, &dict_iter);
    while (dbus_message_iter_get_arg_type(&dict_iter) ==
           DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &key);
        dbus_message_iter_next(&entry_iter);
        dbus_message_iter_recurse(&entry_iter, &variant_iter);

        if (strcmp(key, "type") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &type);
        } else if (strcmp(key, "url") == 0) {
            dbus_message_iter_get_basic(&variant_iter, &url);
        } else {
            return -1;
        }

        dbus_message_iter_next(&dict_iter);
    }

    rc = aui_player_set_subtitle_url(type, url);
    LOGD(TAG, "%s, %d, rc = %d, type = %d, url = %s", __FUNCTION__, __LINE__, rc, type, url);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_subtitle_visible(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_set_subtitle_visible(type, (uint8_t)val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_video_visible(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_set_video_visible(type, (uint8_t)val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_video_crop(DBusConnection *conn, DBusMessage *msg)
{
    xwindow_t win;
    int rc, type = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param1(msg, &type, &win);
    if (rc == 0) {
        rc = aui_player_set_video_crop(type, &win);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, x = %d, y = %d, width = %d, height = %d", __FUNCTION__, __LINE__, rc, type, win.x, win.y, win.width, win.height);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_display_window(DBusConnection *conn, DBusMessage *msg)
{
    xwindow_t win;
    int rc, type = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param1(msg, &type, &win);
    if (rc == 0) {
        rc = aui_player_set_display_window(type, &win);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, x = %d, y = %d, width = %d, height = %d", __FUNCTION__, __LINE__, rc, type, win.x, win.y, win.width, win.height);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_fullscreen(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_set_fullscreen(type, (uint8_t)val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_display_format(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_set_display_format(type, (xdisplay_format_t)val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}

static int _media_dbus_method_set_video_rotate(DBusConnection *conn, DBusMessage *msg)
{
    int rc;
    int type = 0, val = 0;

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    rc = __media_dbus_get_var_param0(msg, &type, &val);
    if (rc == 0) {
        rc = aui_player_set_video_rotate(type, (xrotate_type_t)val);
        LOGD(TAG, "%s, %d, rc = %d, type = %d, val = %d", __FUNCTION__, __LINE__, rc, type, val);
    }

    return kdbus_set_retval_int32(conn, msg, rc);
}
#endif

static const struct kdbus_method_desc g_media_dbus_global_methods[] = {
    {
        MEDIA_DBUS_METHOD_PLAY, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_play,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_PAUSE, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_pause,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_RESUME, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_resume,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_STOP, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_stop,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SEEK, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_seek,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_GET_VOL, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_get_vol,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_VOL, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_vol,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_SPEED, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_speed,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_GET_SPEED, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_get_speed,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_MUTE, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_mute,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_UNMUTE, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_unmute,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_GET_TIME, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_get_time,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_GET_MEDIA_INFO, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_get_media_info,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_GET_STATUS, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_get_status,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
#if !CONFIG_AV_AUDIO_ONLY_SUPPORT
    {
        MEDIA_DBUS_METHOD_SWITCH_AUDIO_TRACK, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_switch_audio_track,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SWITCH_SUBTITLE_TRACK, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_switch_subtitle_track,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_SUBTITLE_URL, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_subtitle_url,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_SUBTITLE_VISIBLE, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_subtitle_visible,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_VIDEO_VISIBLE, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_video_visible,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_VIDEO_CROP, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_video_crop,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_DISPLAY_WINDOW, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_display_window,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_FULLSCREEN, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_fullscreen,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_DISPLAY_FORMAT, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_display_format,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_METHOD_SET_VIDEO_ROTATE, MEDIA_DBUS_INTERFACE,
        (method_function) _media_dbus_method_set_video_rotate,
        {
            { "args", "a{sv}", ARG_IN },
            END_ARGS
        }
    },
#endif
    { NULL, NULL, NULL, { END_ARGS } }
};

static const struct kdbus_signal_desc g_media_dbus_global_signals[] = {
    {
        MEDIA_DBUS_SIGNAL_PLAY_ERROR, MEDIA_DBUS_INTERFACE,
        {
            { "args", "a{sv}", ARG_OUT },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_SIGNAL_PLAY_START, MEDIA_DBUS_INTERFACE,
        {
            { "args", "a{sv}", ARG_OUT },
            END_ARGS
        }
    },
    {
        MEDIA_DBUS_SIGNAL_PLAY_FINISH, MEDIA_DBUS_INTERFACE,
        {
            { "args", "a{sv}", ARG_OUT },
            END_ARGS
        }
    },
    { NULL, NULL, { END_ARGS } }
};

typedef int (*dbus_handler_t)(DBusConnection *conn, DBusMessage *msg);
static struct {
    char            *name;
    dbus_handler_t  handler;
} g_handlers[] = {
    { MEDIA_DBUS_METHOD_PLAY, _media_dbus_method_play },
    { MEDIA_DBUS_METHOD_PAUSE, _media_dbus_method_pause },
    { MEDIA_DBUS_METHOD_RESUME, _media_dbus_method_resume },
    { MEDIA_DBUS_METHOD_STOP, _media_dbus_method_stop },
    { MEDIA_DBUS_METHOD_SEEK, _media_dbus_method_seek },
    { MEDIA_DBUS_METHOD_SET_SPEED, _media_dbus_method_set_speed },
    { MEDIA_DBUS_METHOD_GET_SPEED, _media_dbus_method_get_speed },
    { MEDIA_DBUS_METHOD_GET_STATUS, _media_dbus_method_get_status },
    { MEDIA_DBUS_METHOD_GET_VOL, _media_dbus_method_get_vol },
    { MEDIA_DBUS_METHOD_SET_VOL, _media_dbus_method_set_vol },
    { MEDIA_DBUS_METHOD_MUTE, _media_dbus_method_mute },
    { MEDIA_DBUS_METHOD_UNMUTE, _media_dbus_method_unmute },
    { MEDIA_DBUS_METHOD_GET_TIME, _media_dbus_method_get_time },
    { MEDIA_DBUS_METHOD_GET_MEDIA_INFO, _media_dbus_method_get_media_info },
#if !CONFIG_AV_AUDIO_ONLY_SUPPORT
    { MEDIA_DBUS_METHOD_SWITCH_AUDIO_TRACK, _media_dbus_method_switch_audio_track },
    { MEDIA_DBUS_METHOD_SWITCH_SUBTITLE_TRACK, _media_dbus_method_switch_subtitle_track },
    { MEDIA_DBUS_METHOD_SET_SUBTITLE_URL, _media_dbus_method_set_subtitle_url },
    { MEDIA_DBUS_METHOD_SET_SUBTITLE_VISIBLE, _media_dbus_method_set_subtitle_visible },
    { MEDIA_DBUS_METHOD_SET_VIDEO_VISIBLE, _media_dbus_method_set_video_visible },
    { MEDIA_DBUS_METHOD_SET_VIDEO_CROP, _media_dbus_method_set_video_crop },
    { MEDIA_DBUS_METHOD_SET_DISPLAY_WINDOW, _media_dbus_method_set_display_window },
    { MEDIA_DBUS_METHOD_SET_FULLSCREEN, _media_dbus_method_set_fullscreen },
    { MEDIA_DBUS_METHOD_SET_DISPLAY_FORMAT, _media_dbus_method_set_display_format },
    { MEDIA_DBUS_METHOD_SET_VIDEO_ROTATE, _media_dbus_method_set_video_rotate },
#endif
};

static void _dispatch_msg(DBusConnection *conn, DBusMessage *msg)
{
    int rc, i;
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
        kdbus_introspect(conn, msg, &g_obj_dsc);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(g_handlers); i++) {
        if (dbus_message_is_method_call(msg, MEDIA_DBUS_INTERFACE, g_handlers[i].name)) {
            rc = g_handlers[i].handler(conn, msg);
            LOGI(TAG, "method call: name = %s, rc = %d", g_handlers[i].name, rc);
            return;
        }
    }
    LOGI(TAG, "unknown method call");
}

/**
* @brief  init the config of media server
* @param  [in] conf
* @return 0/-1
*/
int media_server_config_init(msvc_conf_t *conf)
{
    CHECK_PARAM(conf, -1);
    memset(conf, 0, sizeof(msvc_conf_t));

    conf->web_cache_size      = CONFIG_AV_STREAM_CACHE_SIZE_DEFAULT;
    conf->web_start_threshold = CONFIG_AV_STREAM_CACHE_THRESHOLD_DEFAULT;
    conf->snd_period_ms       = AO_ONE_PERIOD_MS;
    conf->snd_period_num      = AO_TOTAL_PERIOD_NUM;
    conf->db_min              = VOL_SCALE_DB_MIN;
    conf->db_max              = VOL_SCALE_DB_MAX;

    return 0;
}

/**
* @brief  init the media server
* @param  [in] conf
* @return 0/-1
*/
int media_server_init(const msvc_conf_t *conf)
{
    int rc = -1;
    DBusError err;
    utask_t *task = NULL;
    DBusConnection *conn = NULL, *conn_listen = NULL;

    CHECK_PARAM(conf && conf->db_min < conf->db_max, -1);
    if (!g_media_server.conn) {
        aui_player_config_t config;

        utask_t *task = utask_new("media-dbus", 8 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
        if (!task) {
            LOGE(TAG, "utask new failed");
            goto err;
        }
        aui_player_init(task, _media_evt);
        aui_player_config_init(&config);
        config.aef_conf            = conf->aef_conf;
        config.aef_conf_size       = conf->aef_conf_size;
        config.resample_rate       = conf->resample_rate;
        config.web_cache_size      = conf->web_cache_size;
        config.web_start_threshold = conf->web_start_threshold;
        config.snd_period_ms       = conf->snd_period_ms;
        config.snd_period_num      = conf->snd_period_num;
        config.vol_map             = conf->vol_map;
        config.db_min              = conf->db_min;
        config.db_max              = conf->db_max;
        aui_player_config(&config);

        g_obj_dsc.methods = g_media_dbus_global_methods;
        g_obj_dsc.signals = g_media_dbus_global_signals;

        dbus_threads_init_default();
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

        rc = dbus_bus_request_name(conn_listen, MEDIA_DBUS_SERVER, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
        if (dbus_error_is_set(&err) || rc != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
            LOGE(TAG, "request name error (%s), rc = %d", err.message, rc);
            goto err;
        }

        dbus_bus_add_match(conn_listen,
                           "type='method_call',interface='"MEDIA_DBUS_INTERFACE"'",
                           &err);
        dbus_connection_flush(conn_listen);
        if (dbus_error_is_set(&err)) {
            LOGE(TAG, "Match Error (%s)", err.message);
            goto err;
        }
        aos_mutex_new(&g_media_server.lock);
        g_media_server.conn        = conn;
        g_media_server.conn_listen = conn_listen;
    }

    return 0;
err:
    if (task)
        utask_destroy(task);
    if (conn) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
        g_media_server.conn = NULL;
    }
    if (conn_listen) {
        dbus_connection_close(conn_listen);
        dbus_connection_unref(conn_listen);
        g_media_server.conn_listen = NULL;
    }
    dbus_error_free(&err);
    return -1;
}

static void _media_ser_task(void *arg)
{
    DBusMessage *msg;
    DBusConnection *conn = get_conn_listen();

    while (!g_media_server.need_quit) {
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
 * @brief  start the media server
 * @return 0/-1
 */
int media_server_start()
{
    int rc;

    if (!g_media_server.start) {
        rc = aos_task_new_ext(&g_media_server.media_ser_task, "media_ser_task", _media_ser_task, NULL, 6 * 1024, AOS_DEFAULT_APP_PRI);
        if (rc) {
            LOGE(TAG, "media server task creae fail, rc = %d", rc);
            return -1;
        }

        g_media_server.start     = 1;
        g_media_server.need_quit = 0;
    }
    return 0;
}

/**
 * @brief  stop the media server
 * @return 0/-1
 */
int media_server_stop()
{
    int rc = -1;
    //TODO:
    return rc;
}

/**
 * @brief  destroy the media server
 * @return 0/-1
 */
int media_server_deinit()
{
    int rc = -1;
    //TODO:
    return rc;
}

#endif

