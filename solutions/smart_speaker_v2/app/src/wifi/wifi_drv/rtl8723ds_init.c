/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <board_config.h>
#include <devices/wifi.h>
#if (defined CONFIG_WIFI_DRIVER_RTL8733) && CONFIG_WIFI_DRIVER_RTL8733
#include <devices/rtl8733bs.h>
#else
#include <devices/rtl8723ds.h>
#endif

int app_wifi_driver_init(void)
{

    /* init wifi driver and network */
#if (defined CONFIG_WIFI_DRIVER_RTL8733) && CONFIG_WIFI_DRIVER_RTL8733
    rtl8733bs_gpio_pin pin = {
        .wl_en = WLAN_ENABLE_PIN,
        .power = WLAN_POWER_PIN,
    };
    wifi_rtl8733bs_register(&pin);
#else
    rtl8723ds_gpio_pin pin = {
        .wl_en = WLAN_ENABLE_PIN,
        .power = WLAN_POWER_PIN,
    };
    wifi_rtl8723ds_register(&pin);
#endif
    return 0;
}

