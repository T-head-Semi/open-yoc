/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <wifi/app_net.h>
#include <ulog/ulog.h>
#include "sys/app_sys.h"
#include "player/app_player.h"

#define TAG "appwifi"

void network_init()
{
    wifi_mode_e mode = app_network_check();
    LOGD(TAG, "WIFI mode %d, Boot Reason %d", mode, app_sys_get_boot_reason());
#if defined(CONFIG_AV_AEF_DEBUG) && CONFIG_AV_AEF_DEBUG
    /* EQ调试模式，停止提示音播放 */
#else
    if (mode != MODE_WIFI_TEST && mode != MODE_WIFI_PAIRING)
    {
        if (app_sys_get_boot_reason() != BOOT_REASON_WIFI_CONFIG &&
            app_sys_get_boot_reason() != BOOT_REASON_WAKE_STANDBY)
        {
            local_audio_play(LOCAL_AUDIO_STARTING);
        }
    }
#endif

    app_network_init();
}

