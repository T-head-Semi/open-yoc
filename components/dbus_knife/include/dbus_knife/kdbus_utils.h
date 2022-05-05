/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#ifndef __KDBUS_UTILS_H__
#define __KDBUS_UTILS_H__

#include <dbus/dbus.h>
#include "dbus_knife/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  get return value of int32 for dbus method-call
 * @param  [in] pending
 * @param  [in] oval : out value of int32
 * @return 0/-1
 */
int kdbus_get_retval_int32(DBusPendingCall *pending, int *oval);

/**
 * @brief  set return value of int32 for dbus method-call
 * @param  [in] conn
 * @param  [in] msg
 * @param  [in] val : return value
 * @return 0/-1
 */
int kdbus_set_retval_int32(DBusConnection *conn, DBusMessage *msg, int val);

/**
 * @brief  get string by dbus-type
 * @param  [in] type : DBUS_TYPE_XXX
 * @return NULL on error
 */
const char *kdbus_get_type_as_string(int type);

/**
 * @brief  set return value of basic for dbus method-call
 * @param  [in] conn
 * @param  [in] msg
 * @param  [in] type : DBUS_TYPE_XXX
 * @param  [in] val : addr of the return value
 * @return 0/-1
 */
int kdbus_set_retval_basic(DBusConnection *conn, DBusMessage *msg, int type, void *val);

#ifdef __cplusplus
}
#endif

#endif /* __KDBUS_UTILS_H__ */

