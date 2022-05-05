/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <ulog/ulog.h>

#ifndef PROJECT_SW_VERSION
#define PROJECT_SW_VERSION 0x00010000
#endif

#define TAG "OTA_VERSION"

static int version_char_to_u32(char* ver_str,unsigned int *version)
{
    int v1 = 0,v2 = 0,v3 = 0;
    if(*ver_str == 'v') {
        sscanf(ver_str, "v%d.%d.%d", &v1,&v2, &v3);
    } else {
        sscanf(ver_str, "%d.%d.%d", &v1,&v2, &v3);
    }
    *version = (v1 << 16 ) | (v2 << 8) | (v3);
    return 0;
}


extern char *aos_get_app_version(void);

unsigned int ota_version_appver_get(void)
{
    unsigned int version = 0;
    int ret = 0;
    char* version_str = aos_get_app_version();
    if(!version_str) {
        LOGE(TAG,"get version faild\r\n");
    }
    ret = version_char_to_u32(version_str,&version);
    if(ret < 0) {
        LOGE(TAG,"trans version faild %d\r\n",ret);
        return 0;
    }
    return version;
}
