/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#include "dbus_knife/kdbus_utils.h"

#define TAG                    "dbus_utils"

/**
 * @brief  get string by dbus-type
 * @param  [in] type : DBUS_TYPE_XXX
 * @return NULL on error
 */
const char *kdbus_get_type_as_string(int type)
{
    switch (type) {
    case DBUS_TYPE_BYTE:
        return DBUS_TYPE_BYTE_AS_STRING;
    case DBUS_TYPE_BOOLEAN:
        return DBUS_TYPE_BOOLEAN_AS_STRING;
    case DBUS_TYPE_INT16:
        return DBUS_TYPE_INT16_AS_STRING;
    case DBUS_TYPE_UINT16:
        return DBUS_TYPE_UINT16_AS_STRING;
    case DBUS_TYPE_INT32:
        return DBUS_TYPE_INT32_AS_STRING;
    case DBUS_TYPE_UINT32:
        return DBUS_TYPE_UINT32_AS_STRING;
    case DBUS_TYPE_INT64:
        return DBUS_TYPE_INT64_AS_STRING;
    case DBUS_TYPE_UINT64:
        return DBUS_TYPE_UINT64_AS_STRING;
    case DBUS_TYPE_DOUBLE:
        return DBUS_TYPE_DOUBLE_AS_STRING;
    case DBUS_TYPE_STRING:
        return DBUS_TYPE_STRING_AS_STRING;
    case DBUS_TYPE_OBJECT_PATH:
        return DBUS_TYPE_OBJECT_PATH_AS_STRING;
    case DBUS_TYPE_ARRAY:
        return DBUS_TYPE_ARRAY_AS_STRING;
    default:
        return NULL;
    }
}

/**
 * @brief  get return value of int32 for dbus method-call
 * @param  [in] pending
 * @param  [in] oval : out value of int32
 * @return 0/-1
 */
int kdbus_get_retval_int32(DBusPendingCall *pending, int *oval)
{
    int rc = -1;
    DBusMessage *msg;
    DBusMessageIter iter;

    CHECK_PARAM(pending && oval, -1);
    dbus_pending_call_block(pending);
    msg = dbus_pending_call_steal_reply(pending);
    CHECK_RET_TAG_WITH_RET(msg, -1);

    dbus_pending_call_unref(pending);
    if (!dbus_message_iter_init(msg, &iter)) {
        LOGE(TAG, "Message has no arguments!");
    } else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&iter)) {
        LOGE(TAG, "Argument is not int32!");
    } else {
        dbus_message_iter_get_basic(&iter, oval);
        rc = 0;
    }

    dbus_message_unref(msg);

    return rc;
}

/**
 * @brief  set return value of int32 for dbus method-call
 * @param  [in] conn
 * @param  [in] msg
 * @param  [in] val : return value
 * @return 0/-1
 */
int kdbus_set_retval_int32(DBusConnection *conn, DBusMessage *msg, int val)
{
    int rc = -1;
    DBusMessage *reply;
    dbus_uint32_t serial = 0;
    DBusMessageIter iter = {0};

    CHECK_PARAM(conn && msg, -1);
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &val);

    if (!dbus_connection_send(conn, reply, &serial)) {
        LOGE(TAG, "send failed");
        goto err;
    }
    dbus_connection_flush(conn);
    rc = 0;

err:
    dbus_message_unref(reply);
    return rc;
}

/**
 * @brief  set return value of basic for dbus method-call
 * @param  [in] conn
 * @param  [in] msg
 * @param  [in] type : DBUS_TYPE_XXX
 * @param  [in] val : addr of the return value
 * @return 0/-1
 */
int kdbus_set_retval_basic(DBusConnection *conn, DBusMessage *msg, int type, void *val)
{
    int rc = -1;
    DBusMessage *reply;
    dbus_uint32_t serial = 0;
    DBusMessageIter iter = {0};

    CHECK_PARAM(conn && msg && type && val, -1);
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_append_basic(&iter, type, val);

    if (!dbus_connection_send(conn, reply, &serial)) {
        LOGE(TAG, "send failed");
        goto err;
    }
    dbus_connection_flush(conn);
    rc = 0;

err:
    dbus_message_unref(reply);
    return rc;
}


