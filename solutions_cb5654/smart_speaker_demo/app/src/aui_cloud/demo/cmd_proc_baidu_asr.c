/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "aui_nlp.h"

#define TAG "app"

/**
 * 接收到百度ASR的文本结果，调用aui_cloud_push_text进行语义解析
*/
int aui_nlp_process_baidu_asr(cJSON *js, const char *json_text)
{
    LOGD(TAG, "Enter %s", __FUNCTION__);
    
    cJSON *results  = cJSON_GetObjectItem(js, "results_recognition");

    if (cJSON_GetArraySize(results) <=0) {
        LOGD(TAG, "results not found");
        return -1;
    }

    cJSON *result_item = cJSON_GetArrayItem(results, 0);
    if (!cJSON_IsString(result_item)) {
        LOGD(TAG, "results type error");
        return -1;
    }

    return app_aui_cloud_push_text(result_item->valuestring);
}
