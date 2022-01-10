/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef _VOICE_CLIENT_HH_
#define _VOICE_CLIENT_HH_

#include <voice_hal.h>

namespace Voice
{

class VoiceClient
{
public:
    int start(void);
    int stop(void);
    const char *get_state(void);
    int pcm_config(csi_pcm_format_t *format);
    int alg_config(void *config, int size);
    int asr(char *pcm, int size);
    int nlp(const char *text);
    int tts(const char *text);

    VoiceClient();
    ~VoiceClient();
};

}

#endif
