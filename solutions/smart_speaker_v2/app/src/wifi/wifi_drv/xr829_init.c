/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <devices/wifi.h>
#include <devices/xr829.h>

int app_wifi_driver_init(void)
{
    wifi_xr829_register(NULL);
    return 0;
}

