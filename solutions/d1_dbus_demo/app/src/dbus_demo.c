/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#include "dbus_knife/kdbus_utils.h"
#include "dbus_knife/kdbus_introspect.h"
#include "dbus_demo.h"

#define TAG                   "dbus_demo"

static struct kdbus_object_desc g_obj_dsc;

static struct {
    int                       need_quit;
    int                       start;
    DBusConnection            *conn;
    DBusConnection            *conn_listen;
    aos_task_t                dbus_demo_task;
} g_dbus_demo;
static int g_switch_status;

#define get_conn()            g_dbus_demo.conn
#define get_conn_listen()     g_dbus_demo.conn_listen

static int _dbus_demo_send_switch_event(int status)
{
    int rc = -1;
    char *key1 = "status";
    DBusMessage *msg;
    dbus_uint32_t serial = 0;
    DBusConnection *conn = get_conn();
    DBusMessageIter iter = {0};
    DBusMessageIter iter_dict = {0}, variant_iter = {0};
    DBusMessageIter iter_dict_entry = {0}, iter_dict_val = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    msg = dbus_message_new_signal(DEMO_DBUS_PATH,
                                  DEMO_DBUS_INTERFACE, DEMO_DBUS_SIGNAL_SWITCH_STATUS);
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
    dbus_message_iter_append_basic(&iter_dict_val, DBUS_TYPE_INT32, &status);
    dbus_message_iter_close_container(&iter_dict_entry, &iter_dict_val);
    dbus_message_iter_close_container(&iter_dict, &iter_dict_entry);

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

static int _dbus_demo_method_switch(DBusConnection *conn, DBusMessage *msg)
{
    int rc = 0, action;
    DBusMessageIter iter = {0};

    LOGI(TAG, "%s, %d enter", __FUNCTION__, __LINE__);
    dbus_message_iter_init(msg, &iter);
    dbus_message_iter_get_basic(&iter, &action);

    g_switch_status = action;
    LOGD(TAG, "%s, %d, switch status = %s", __FUNCTION__, __LINE__, action == 1 ? "on" : "off");
    _dbus_demo_send_switch_event(g_switch_status);

    return kdbus_set_retval_int32(conn, msg, rc);
}

static const struct kdbus_method_desc g_dbus_demo_global_methods[] = {
    {
        DEMO_DBUS_METHOD_SWITCH, DEMO_DBUS_INTERFACE,
        (method_function) _dbus_demo_method_switch,
        {
            { "type", "i", ARG_IN },
            END_ARGS
        }
    },
    { NULL, NULL, NULL, { END_ARGS } }
};

static const struct kdbus_signal_desc g_dbus_demo_global_signals[] = {
    {
        DEMO_DBUS_SIGNAL_SWITCH_STATUS, DEMO_DBUS_INTERFACE,
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
    { DEMO_DBUS_METHOD_SWITCH, _dbus_demo_method_switch },
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
        if (dbus_message_is_method_call(msg, DEMO_DBUS_INTERFACE, g_handlers[i].name)) {
            rc = g_handlers[i].handler(conn, msg);
            LOGI(TAG, "method call: name = %s, rc = %d", g_handlers[i].name, rc);
            return;
        }
    }
    LOGI(TAG, "unknown method call");
}

int _dbus_demo_init()
{
    int rc = -1;
    DBusError err;
    DBusConnection *conn = NULL, *conn_listen = NULL;

    if (!g_dbus_demo.conn) {
        g_obj_dsc.methods = g_dbus_demo_global_methods;
        g_obj_dsc.signals = g_dbus_demo_global_signals;

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

        rc = dbus_bus_request_name(conn_listen, DEMO_DBUS_SERVER, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
        if (dbus_error_is_set(&err) || rc != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
            LOGE(TAG, "request name error (%s), rc = %d", err.message, rc);
            goto err;
        }

        dbus_bus_add_match(conn_listen,
                           "type='method_call',interface='"DEMO_DBUS_INTERFACE"'",
                           &err);
        dbus_connection_flush(conn_listen);
        if (dbus_error_is_set(&err)) {
            LOGE(TAG, "Match Error (%s)", err.message);
            goto err;
        }
        g_dbus_demo.conn        = conn;
        g_dbus_demo.conn_listen = conn_listen;
    }

    return 0;
err:
    if (conn) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
        g_dbus_demo.conn = NULL;
    }
    if (conn_listen) {
        dbus_connection_close(conn_listen);
        dbus_connection_unref(conn_listen);
        g_dbus_demo.conn_listen = NULL;
    }
    dbus_error_free(&err);
    return -1;
}

static void _dbus_demo_task(void *arg)
{
    DBusMessage *msg;
    DBusConnection *conn = get_conn_listen();

    while (!g_dbus_demo.need_quit) {
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
 * @brief  start the dbus demo
 * @return 0/-1
 */
int dbus_demo_start()
{
    int rc;

    if (!g_dbus_demo.start) {
        rc = _dbus_demo_init();
        if (rc) {
            LOGE(TAG, "dbus demo init fail, rc = %d", rc);
            return -1;
        } else {
            rc = aos_task_new_ext(&g_dbus_demo.dbus_demo_task, "dbus_demo_task", _dbus_demo_task, NULL, 6 * 1024, AOS_DEFAULT_APP_PRI);
            if (rc) {
                LOGE(TAG, "dbus demo dbus_demo_task create fail, rc = %d", rc);
                return -1;
            }

            g_dbus_demo.start     = 1;
            g_dbus_demo.need_quit = 0;

        }
    }
    return 0;
}

