## 概述

基于aui_cloud组件实现的mit asr/tts/nlp相关算法。

## 配置

无。

## 接口列表

| 函数                 | 说明                        |
| :------------------- | :-------------------------- |
| aui_asr_register | mit的asr算法注册到aui_cloud |
| aui_tts_register | mit的tts算法注册到aui_cloud |

## 接口详细说明

### mit的asr算法注册到aui_cloud

`void aui_asr_register(aui_t *aui, aui_asr_cb_t cb, void *priv);`

- 参数:
  - aui: aui 句柄
  - cb：ars事件的callback
  - priv：用户参数
- 返回值:
  - 无

### mit的tts算法注册到aui_cloud

`void aui_tts_register(aui_t *aui, aui_tts_cb_tcb, void *priv);`

- 参数:
  - aui: aui 句柄
  - cb：ars事件的callback
  - priv：用户参数
- 返回值:
  - 无

## 示例

```c
int app_aui_init(void)
{
    int ret = 0;
    cJSON *js_account_info = NULL;
    char *s_account_info = NULL;
    char device_uuid[100] = {0};

    get_uuid(device_uuid);

    aos_event_new(&event_tts_state, 0);
    // event_subscribe(EVENT_MEDIA_SYSTEM_ERROR, media_state_cb, NULL);
    // event_subscribe(EVENT_MEDIA_SYSTEM_FINISH, media_state_cb, NULL);

    js_account_info = cJSON_CreateObject();
    CHECK_RET_WITH_GOTO(js_account_info, END);


     /*ASR kaishu test account*/
    cJSON_AddStringToObject(js_account_info, "device_uuid", device_uuid);
    cJSON_AddStringToObject(js_account_info, "asr_app_key", "g3aHMdL7v63bZCS3");
    cJSON_AddStringToObject(js_account_info, "asr_token", "4a37aa0c0373498ea04f732054841b62");
    cJSON_AddStringToObject(js_account_info, "asr_url", "wss://nls-gateway-inner.aliyuncs.com/ws/v1");
    
    /*TTS*/
    cJSON_AddStringToObject(js_account_info, "tts_app_key", "9a7f47f2");
    cJSON_AddStringToObject(js_account_info, "tts_token", "a2f8b80e04f14fdb9b7c36024fb03f78");
    cJSON_AddStringToObject(js_account_info, "tts_url", "wss://nls-gateway-inner.aliyuncs.com/ws/v1");

    /*TTS*/
    cJSON_AddStringToObject(js_account_info, "tts_app_key", "9a7f47f2");
    cJSON_AddStringToObject(js_account_info, "tts_token", "a2f8b80e04f14fdb9b7c36024fb03f78");
    cJSON_AddStringToObject(js_account_info, "tts_url", "wss://nls-gateway-inner.aliyuncs.com/ws/v1");
    
    s_account_info = cJSON_PrintUnformatted(js_account_info);
    CHECK_RET_TAG_WITH_GOTO(s_account_info, END);

    aui_config_t cfg;
    cfg.per             = "aixia";
    cfg.vol             = 100;      /* 音量 0~100 */
    cfg.spd             = 0;        /* -500 ~ 500*/
    cfg.pit             = 0;        /* 音调*/
    cfg.asr_fmt         = 2;        /* 编码格式，1：PCM 2：MP3 */
    cfg.tts_fmt         = 2;        /* 编码格式，1：PCM 2：MP3 */
    cfg.srate           = 16000;    /* 采样率，16000 */
    cfg.tts_cache_path  = NULL;     /* TTS内部缓存路径，NULL：关闭缓存功能 */
    cfg.cloud_vad       = 1;        /* 云端VAD功能使能， 0：关闭；1：打开 */
    cfg.js_account      = s_account_info;

    g_aui_handler = aui_cloud_init(&cfg);
    mit_tts_fifo = cfg.tts_fmt == 1 ? "fifo://mittts?avformat=rawaudio&avcodec=pcm_s16le&channel=1&rate=16000" : "fifo://mittts";

    aui_asr_register(g_aui_handler, mit_asr_handle, NULL);
    aui_tts_register(g_aui_handler, mit_tts_handle, NULL);
    aos_event_new(&event_wwv_result, 0);

    if (ret != 0) {
        LOGE(TAG, "ai engine error");
        goto END;
    }

END:
    cJSON_Delete(js_account_info);
    free(s_account_info);
    
    return ret;
}
```



## 诊断错误码

无。

## 运行资源

无。

## 依赖资源

minilibc: v7.2.0及以上。
aos: v7.2.0及以上。

## 组件参考

无。