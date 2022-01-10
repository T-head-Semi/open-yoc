/*
 * wpa_supplicant - D-Bus introspection
 * Copyright (c) 2006, Dan Williams <dcbw@redhat.com> and Red Hat, Inc.
 * Copyright (c) 2009, Witold Sowa <witold.sowa@gmail.com>
 * Copyright (c) 2010, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "dbus_knife/list.h"
#include "dbus_knife/kdbus_buf.h"
#include "dbus_knife/kdbus_new_helpers.h"
#include "dbus_knife/kdbus_introspect.h"

#define kdbus_printf(...)

#define TAG                   "kdbus_introspect"

struct interfaces {
    struct dl_list     list;
    char               *dbus_interface;
    struct kdbus_buf    *xml;
};

static struct interfaces *_add_interface(struct dl_list *list,
        const char *dbus_interface)
{
    struct interfaces *iface;

    dl_list_for_each(iface, list, struct interfaces, list) {
        if (os_strcmp(iface->dbus_interface, dbus_interface) == 0) {
            return iface; /* already in the list */
        }
    }

    iface = aos_zalloc(sizeof(struct interfaces));

    if (!iface) {
        return NULL;
    }

    iface->dbus_interface = os_strdup(dbus_interface);
    iface->xml = kdbus_buf_alloc(15000);

    if (iface->dbus_interface == NULL || iface->xml == NULL) {
        os_free(iface->dbus_interface);
        kdbus_buf_free(iface->xml);
        os_free(iface);
        return NULL;
    }

    kdbus_buf_printf(iface->xml, "<interface name=\"%s\">", dbus_interface);
    dl_list_add_tail(list, &iface->list);
    return iface;
}


static void _add_arg(struct kdbus_buf *xml, const char *name, const char *type,
                     const char *direction)
{
    kdbus_buf_printf(xml, "<arg name=\"%s\"", name);

    if (type) {
        kdbus_buf_printf(xml, " type=\"%s\"", type);
    }

    if (direction) {
        kdbus_buf_printf(xml, " direction=\"%s\"", direction);
    }

    kdbus_buf_put_str(xml, "/>");
}


static void _add_entry(struct kdbus_buf *xml, const char *type, const char *name,
                       const struct kdbus_argument *args, int include_dir)
{
    const struct kdbus_argument *arg;

    if (args == NULL || args->name == NULL) {
        kdbus_buf_printf(xml, "<%s name=\"%s\"/>", type, name);
        return;
    }

    kdbus_buf_printf(xml, "<%s name=\"%s\">", type, name);

    for (arg = args; arg && arg->name; arg++) {
        _add_arg(xml, arg->name, arg->type,
                 include_dir ? (arg->dir == ARG_IN ? "in" : "out") :
                 NULL);
    }

    kdbus_buf_printf(xml, "</%s>", type);
}


static void _add_property(struct kdbus_buf *xml,
                          const struct kdbus_property_desc *dsc)
{
    kdbus_buf_printf(xml, "<property name=\"%s\" type=\"%s\" "
                     "access=\"%s%s\"/>",
                     dsc->dbus_property, dsc->type,
                     dsc->getter ? "read" : "",
                     dsc->setter ? "write" : "");
}


static void _extract_interfaces_methods(
    struct dl_list *list, const struct kdbus_method_desc *methods)
{
    const struct kdbus_method_desc *dsc;
    struct interfaces *iface;

    for (dsc = methods; dsc && dsc->dbus_method; dsc++) {
        iface = _add_interface(list, dsc->dbus_interface);

        if (iface) {
            _add_entry(iface->xml, "method", dsc->dbus_method,
                       dsc->args, 1);
        }
    }
}


static void _extract_interfaces_signals(
    struct dl_list *list, const struct kdbus_signal_desc *signals)
{
    const struct kdbus_signal_desc *dsc;
    struct interfaces *iface;

    for (dsc = signals; dsc && dsc->dbus_signal; dsc++) {
        iface = _add_interface(list, dsc->dbus_interface);

        if (iface)
            _add_entry(iface->xml, "signal", dsc->dbus_signal,
                       dsc->args, 0);
    }
}


static void _extract_interfaces_properties(
    struct dl_list *list, const struct kdbus_property_desc *properties)
{
    const struct kdbus_property_desc *dsc;
    struct interfaces *iface;

    for (dsc = properties; dsc && dsc->dbus_property; dsc++) {
        iface = _add_interface(list, dsc->dbus_interface);

        if (iface) {
            _add_property(iface->xml, dsc);
        }
    }
}


/**
 * _extract_interfaces - Extract interfaces from methods, signals and props
 * @list: Interface list to be filled
 * @obj_dsc: Description of object from which interfaces will be extracted
 *
 * Iterates over all methods, signals, and properties registered with an
 * object and collects all declared DBus interfaces and create interfaces'
 * node in XML root node for each. Returned list elements contain interface
 * name and XML node of corresponding interface.
 */
static void _extract_interfaces(struct dl_list *list,
                                struct kdbus_object_desc *obj_dsc)
{
    _extract_interfaces_methods(list, obj_dsc->methods);
    _extract_interfaces_signals(list, obj_dsc->signals);
    _extract_interfaces_properties(list, obj_dsc->properties);
}


static void _add_interfaces(struct dl_list *list, struct kdbus_buf *xml)
{
    struct interfaces *iface, *n;

    dl_list_for_each_safe(iface, n, list, struct interfaces, list) {
        if (kdbus_buf_len(iface->xml) + 20 < kdbus_buf_tailroom(xml)) {
            kdbus_buf_put_buf(xml, iface->xml);
            kdbus_buf_put_str(xml, "</interface>");
        } else {
            kdbus_printf(MSG_DEBUG,
                         "dbus: Not enough room for _add_interfaces inspect data: tailroom %u, add %u",
                         (unsigned int) kdbus_buf_tailroom(xml),
                         (unsigned int) kdbus_buf_len(iface->xml));
        }

        dl_list_del(&iface->list);
        kdbus_buf_free(iface->xml);
        os_free(iface->dbus_interface);
        os_free(iface);
    }
}


static void _add_child_nodes(struct kdbus_buf *xml, DBusConnection *con,
                             const char *path)
{
    char **children;
    int i;

    /* add child nodes to introspection tree */
    dbus_connection_list_registered(con, path, &children);

    for (i = 0; children[i]; i++) {
        kdbus_buf_printf(xml, "<node name=\"%s\"/>", children[i]);
    }

    dbus_free_string_array(children);
}


static void _add_introspectable_interface(struct kdbus_buf *xml)
{
    kdbus_buf_printf(xml, "<interface name=\"%s\">"
                     "<method name=\"%s\">"
                     "<arg name=\"data\" type=\"s\" direction=\"out\"/>"
                     "</method>"
                     "</interface>",
                     KDBUS_INTROSPECTION_INTERFACE,
                     KDBUS_INTROSPECTION_METHOD);
}


static void _add_properties_interface(struct kdbus_buf *xml)
{
    kdbus_buf_printf(xml, "<interface name=\"%s\">",
                     KDBUS_PROPERTIES_INTERFACE);

    kdbus_buf_printf(xml, "<method name=\"%s\">", KDBUS_PROPERTIES_GET);
    _add_arg(xml, "interface", "s", "in");
    _add_arg(xml, "propname", "s", "in");
    _add_arg(xml, "value", "v", "out");
    kdbus_buf_put_str(xml, "</method>");

    kdbus_buf_printf(xml, "<method name=\"%s\">", KDBUS_PROPERTIES_GETALL);
    _add_arg(xml, "interface", "s", "in");
    _add_arg(xml, "props", "a{sv}", "out");
    kdbus_buf_put_str(xml, "</method>");

    kdbus_buf_printf(xml, "<method name=\"%s\">", KDBUS_PROPERTIES_SET);
    _add_arg(xml, "interface", "s", "in");
    _add_arg(xml, "propname", "s", "in");
    _add_arg(xml, "value", "v", "in");
    kdbus_buf_put_str(xml, "</method>");

    kdbus_buf_put_str(xml, "</interface>");
}


static void _add_kdbuss_interfaces(struct kdbus_buf *xml,
                                   struct kdbus_object_desc *obj_dsc)
{
    struct dl_list ifaces;

    dl_list_init(&ifaces);
    _extract_interfaces(&ifaces, obj_dsc);
    _add_interfaces(&ifaces, xml);
}


/**
 * kdbus_introspect - Responds for Introspect calls on object
 * @message: Message with Introspect call
 * @obj_dsc: Object description on which Introspect was called
 * Returns: 0/-1
 *
 * Iterates over all methods, signals and properties registered with
 * object and generates introspection data for the object as XML string.
 */
int kdbus_introspect(DBusConnection *conn, DBusMessage *msg, struct kdbus_object_desc *obj_dsc)
{
    DBusMessage *reply;
    struct kdbus_buf *xml;

    CHECK_PARAM(conn && msg, -1);
    xml = kdbus_buf_alloc(20000);
    CHECK_RET_TAG_WITH_RET(xml, -1);

    kdbus_buf_put_str(xml, "<?xml version=\"1.0\"?>\n");
    kdbus_buf_put_str(xml, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
    kdbus_buf_put_str(xml, "<node>");

    _add_introspectable_interface(xml);
    _add_properties_interface(xml);
    _add_kdbuss_interfaces(xml, obj_dsc);
    _add_child_nodes(xml, conn, dbus_message_get_path(msg));

    kdbus_buf_put_str(xml, "</node>\n");

    reply = dbus_message_new_method_return(msg);
    if (reply) {
        const char *intro_str = kdbus_buf_head(xml);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &intro_str,
                                 DBUS_TYPE_INVALID);
        if (!dbus_message_get_no_reply(msg)) {
            dbus_connection_send(conn, reply, NULL);
        }
        dbus_message_unref(reply);
        dbus_connection_flush(conn);
    }

    kdbus_buf_free(xml);

    return 0;
}
