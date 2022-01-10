/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */


#include <voice_client.hh>

#include <iostream>

using std::string;
using std::cout;
using std::endl;
using std::ifstream;

using namespace Voice;

VoiceClient::VoiceClient()
{

}

VoiceClient::~VoiceClient()
{

}

int VoiceClient::start(void)
{
    return 0;
}

int VoiceClient::stop(void)
{
    return 0;
}

const char *VoiceClient::get_state(void)
{
    return 0;
}

int VoiceClient::pcm_config(csi_pcm_format_t *format)
{
    return 0;
}

int VoiceClient::alg_config(void *config, int size)
{
    return 0;
}

int VoiceClient::asr(char *pcm, int size)
{
    return 0;
}

int VoiceClient::nlp(const char *text)
{
    return 0;
}

int VoiceClient::tts(const char *text)
{
    return 0;
}
