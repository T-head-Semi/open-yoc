/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#ifndef __DBUS_DEMO_H__
#define __DBUS_DEMO_H__

#include "dbus_knife/common.h"

__BEGIN_DECLS__

#define DEMO_DBUS_SERVER                           "org.dbus_demo.server"
#define DEMO_DBUS_PATH                             "/org/dbus_demo/path"
#define DEMO_DBUS_INTERFACE                        "org.dbus_demo.interface"

#define DEMO_DBUS_METHOD_SWITCH                    "Switch"

#define DEMO_DBUS_SIGNAL_SWITCH_STATUS             "SwitchStatus"

/**
 * @brief  start the dbus demo
 * @return 0/-1
 */
int dbus_demo_start();

__END_DECLS__

#endif /* __DBUS_DEMO_H__ */

