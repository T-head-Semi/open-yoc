/*
 * KDBUS Supplicant / dbus-based control interface
 * Copyright (c) 2006, Dan Williams <dcbw@redhat.com> and Red Hat, Inc.
 * Copyright (c) 2009, Witold Sowa <witold.sowa@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef _KDBUS_NEW_HELPER_H_
#define _KDBUS_NEW_HELPER_H_

#include <stdint.h>
#include <dbus/dbus.h>

#define KDBUS_INTROSPECTION_INTERFACE                "org.freedesktop.DBus.Introspectable"
#define KDBUS_INTROSPECTION_METHOD                   "Introspect"
#define KDBUS_PROPERTIES_INTERFACE                   "org.freedesktop.DBus.Properties"
#define KDBUS_PROPERTIES_GET                         "Get"
#define KDBUS_PROPERTIES_SET                         "Set"
#define KDBUS_PROPERTIES_GETALL                      "GetAll"

typedef int (*method_function)(DBusConnection *conn, DBusMessage *msg);
typedef void (*KDBusArgumentFreeFunction)(void *handler_arg);

struct kdbus_property_desc;
typedef dbus_bool_t (*KDBusPropertyAccessor)(
    const struct kdbus_property_desc *property_desc,
    DBusMessageIter *iter, DBusError *error, void *user_data);
#define DECLARE_ACCESSOR(f) \
    dbus_bool_t f(const struct kdbus_property_desc *property_desc, \
                  DBusMessageIter *iter, DBusError *error, void *user_data)

struct kdbus_object_desc {
    char *path;

    /* list of methods, properties and signals registered with object */
    const struct kdbus_method_desc *methods;
    const struct kdbus_signal_desc *signals;
    const struct kdbus_property_desc *properties;

    /* property changed flags */
    uint8_t *prop_changed_flags;

    /* argument for method handlers and properties
     * getter and setter functions */
    void *user_data;
    /* function used to free above argument */
    KDBusArgumentFreeFunction user_data_free_func;
};

enum kdbus_arg_direction { ARG_IN, ARG_OUT };

/**
 * type Definition:
 *
 * 'Auto':  'v'
 * String:  's'
 * Number:  'd'
 * Boolean: 'b'
 * Array:   'av'
 * Object:  'a{sv}'
 */
struct kdbus_argument {
    char *name;
    char *type;
    enum kdbus_arg_direction dir;
};

#define END_ARGS { NULL, NULL, ARG_IN }

/**
 * struct kdbus_method_desc - DBus method description
 */
struct kdbus_method_desc {
    /* method name */
    const char *dbus_method;
    /* method interface */
    const char *dbus_interface;
    /* method handling function */
    method_function function;
    /* array of arguments */
    struct kdbus_argument args[4];
};

/**
 * struct kdbus_signal_desc - DBus signal description
 */
struct kdbus_signal_desc {
    /* signal name */
    const char *dbus_signal;
    /* signal interface */
    const char *dbus_interface;
    /* array of arguments */
    struct kdbus_argument args[4];
};

/**
 * struct kdbus_property_desc - DBus property description
 */

struct kdbus_property_desc {
    /* property name */
    const char *dbus_property;
    /* property interface */
    const char *dbus_interface;
    /* property type signature in DBus type notation */
    const char *type;
    /* property getter function */
    KDBusPropertyAccessor getter;
    /* property setter function */
    KDBusPropertyAccessor setter;
    /* other data */
    const char *data;
};

#endif /* _KDBUS_NEW_HELPER_H_ */
