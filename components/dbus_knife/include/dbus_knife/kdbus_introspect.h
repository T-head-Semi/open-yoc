/*
 * wpa_supplicant - D-Bus introspection
 * Copyright (c) 2006, Dan Williams <dcbw@redhat.com> and Red Hat, Inc.
 * Copyright (c) 2009, Witold Sowa <witold.sowa@gmail.com>
 * Copyright (c) 2010, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef __KDBUS_INTROSPECT_H__
#define __KDBUS_INTROSPECT_H__

#include <dbus/dbus.h>
#include "dbus_knife/kdbus_new_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * kdbus_introspect - Responds for Introspect calls on object
 * @message: Message with Introspect call
 * @obj_dsc: Object description on which Introspect was called
 * Returns: 0/-1
 *
 * Iterates over all methods, signals and properties registered with
 * object and generates introspection data for the object as XML string.
 */
int kdbus_introspect(DBusConnection *conn, DBusMessage *msg, struct kdbus_object_desc *obj_dsc);

#ifdef __cplusplus
}
#endif

#endif /* __KDBUS_INTROSPECT_H__ */

