/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __APP_VOCE_H
#define __APP_VOCE_H

#include "stdint.h"
#include "./nsp/nsp_driver.h"

typedef enum {
  VOICE_ID_PROVISIONING        = 0x01,
  VOICE_ID_PROVISIONING_FAIL   = 0x02,
  VOICE_ID_PROVISIONING_SUCCESS = 0x03,
} VOICE_PLAY_ID;

void app_voice_play(uint8_t index,uint8_t voice);


#endif
