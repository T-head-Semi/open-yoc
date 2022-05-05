/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <smart_audio.h>

#include "aui_cloud/ex_cjson.h"

#include "aui_nlp.h"

#define TAG "nlptxt"

#define URL_BASE "https://cop-image-prod.oss-cn-hangzhou.aliyuncs.com/resource/undefined"
#define MP3_HZ300 "1577167230775/Sin300Hz.mp3"
#define MP3_HZ1K "1577167215618/Sin1000Hz.mp3"
#define MP3_TEST1 "1577166283488/AudioTest1.mp3"
#define MP3_TEST2 "1577167181404/AudioTest2.mp3"

/**
 * 文本命令解析的处理
*/
int aui_nlp_proc_textcmd(const char *asr_text)
{
    LOGD(TAG, "Enter %s", __FUNCTION__);

    if (asr_text == NULL) {
        LOGD(TAG, "textcmd not found");
        return -1;
    }

    const char *cmd = asr_text;
    LOGD(TAG, "process_textcmd:%s", cmd);
    if (strstr(cmd, "media_close") != NULL) {
        smtaudio_stop(MEDIA_ALL);
    } else if (strstr(cmd, "media_test") != NULL) {
        int id = (rand() % 2) + 1;
        switch(id){
            case 1:
                smtaudio_start(MEDIA_MUSIC, URL_BASE"/"MP3_TEST1, 0, 0);
                break;
            case 2:
                smtaudio_start(MEDIA_MUSIC, URL_BASE"/"MP3_TEST2, 0, 0);
                break;
            default:
                return -1;
        }
    } else if (strstr(cmd, "media_1khz") != NULL) {
        smtaudio_start(MEDIA_MUSIC, URL_BASE"/"MP3_HZ1K, 0, 0);
    } else if (strstr(cmd, "media_300hz") != NULL) {
        smtaudio_start(MEDIA_MUSIC, URL_BASE"/"MP3_HZ300, 0, 0);
    } else {
        LOGW(TAG, "unknown cmd");
        return -1;
    }

    return 0;
}
