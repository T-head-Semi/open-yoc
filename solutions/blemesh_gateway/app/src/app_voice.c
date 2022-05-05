/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "app_voice.h"

void app_voice_play(uint8_t index,uint8_t voice)
{
    HOST_BUS_Init();
    N_PLAY(index);
    N_SET_VOL(voice);
}


