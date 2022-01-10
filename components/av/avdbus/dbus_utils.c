/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AV_DBUS_SUPPORT) && CONFIG_AV_DBUS_SUPPORT
#include "avdbus/dbus_utils.h"

#define TAG                    "dbus_utils"

static struct {
    aui_player_evtid_t        evt_id;
    char                      *name;
} g_evtmap[] = {
    { AUI_PLAYER_EVENT_ERROR, MEDIA_DBUS_SIGNAL_PLAY_ERROR },
    { AUI_PLAYER_EVENT_START, MEDIA_DBUS_SIGNAL_PLAY_START },
    { AUI_PLAYER_EVENT_FINISH, MEDIA_DBUS_SIGNAL_PLAY_FINISH },
    { AUI_PLAYER_EVENT_PAUSE, MEDIA_DBUS_SIGNAL_PLAY_PAUSE },
    { AUI_PLAYER_EVENT_RESUME, MEDIA_DBUS_SIGNAL_PLAY_RESUME },
    { AUI_PLAYER_EVENT_STOP, MEDIA_DBUS_SIGNAL_PLAY_STOP },

    { AUI_PLAYER_EVENT_MUTE, MEDIA_DBUS_SIGNAL_PLAY_MUTE },
    { AUI_PLAYER_EVENT_VOL_CHANGE, MEDIA_DBUS_SIGNAL_PLAY_VOL_CHANGE },
};

/**
 * @brief  get event name by id
 * @param  [in] evt_id
 * @return NULL on error
 */
const char *get_name_by_evtid(aui_player_evtid_t evt_id)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(g_evtmap); i++) {
        if (evt_id == g_evtmap[i].evt_id)
            return g_evtmap[i].name;
    }

    return NULL;
}

#endif



