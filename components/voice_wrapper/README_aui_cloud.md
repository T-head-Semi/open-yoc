## 概述

aui cloud是专为rtos打造的语音解决方案，提供了语音合成、语义理解、本地语义端文本理解等接口。

## 配置

无。

## 接口列表

| 函数                                                         | 说明                                                     |
| :----------------------------------------------------------- | :------------------------------------------------------- |
| aui_cloud_init                                               | AUI系统初始化                                            |
| aui_cloud_deinit                                             | AUI系统去初始化                                          |
| aui_cloud_stop                                               | 停止上云流程                                             |
| aui_cloud_asr_register / aui_cloud_asr_unregister            | 向AUI注册（取消）asr方法                                 |
| aui_cloud_nlp_register / aui_cloud_nlp_unregister            | 向AUI注册（取消）nlp方法                                 |
| aui_cloud_tts_register / aui_cloud_tts_unregister向AUI注册（取消）asr方法 | 向AUI注册（取消）ttx方法                                 |
| aui_cloud_start_audio                                        | 启动语音数据交互                                         |
| aui_cloud_push_audio                                         | 向AUI系统输入语音数据                                    |
| aui_cloud_stop_audio                                         | 结束语音数据输入，AUI系统开始处理                        |
| aui_cloud_start_nlp                                          | 向AUI系统输入文本数据，AUI进行处理并回调NLP处理回调函数  |
| aui_cloud_push_text                                          | 获取字符串类型的KV项                                     |
| aui_cloud_stop_nlp                                           | 停止nlp系统，断开云端连接                                |
| aui_cloud_start_tts                                          | 启动tts系统，连接云端                                    |
| aui_cloud_req_tts                                            | 向AUI系统输入文本数据，要求返回文本的TTS转换后的语音结果 |
| aui_cloud_stop_tts                                           | 停止tts系统，断开云端连接                                |

## 接口详细说明

### AUI系统初始化

```C
aui_t *aui_cloud_init(aui_config_t *config);
```

* 参数
  * config:  aui cloud配置参数
* 返回值
  * 不等于 NULL: 成功
  * 等于 NULL: 失败



```c
typedef struct aui_config {
    char *per;                      // 发音人选择
    int   vol;                      // 音量，取值0-100，50标准音量
    int   pit;                      // 音调，预留配置
    int   spd;                      // 语速，-500~500，默认0标准语速
    int   asr_fmt;                  // asr push audio时的编码格式， 1：pcm 2：mp3
    int   tts_fmt;                  // tts 成功时的audio编码格式，  1：pcm 2：mp3
    int   nlp_fmt;                  // tts 成功时的audio编码格式，  1：pcm 2：mp3
    int   srate;                    // 采样率，预留
    int   cloud_vad;                // 云端VAD功能使能，预留
    char *tts_cache_path;	        // TTS内部缓存路径，预留
    const char *js_account;         // json格式的账号信息，包含aui系统的所有账号信息
} aui_config_t;
```



### AUI系统去初始化

```CQL
int aui_cloud_deinit(aui_t *aui);
```

- 参数
  - aui: `aui_t`指针

- 返回值
  - 0: 成功
  - < 0: 失败

### 停止AUI系统

```c
int aui_cloud_stop(aui_t *aui);
```

- 参数
  - aui: `aui_t`指针

- 返回值
  - 0: 成功
  - < 0: 失败



### AUI系统（取消）注册asr、nlp、tts

```c
int aui_cloud_asr_register(aui_t *aui, aui_asr_cls_t *ops, aui_asr_cb_t cb, void *priv);
int aui_cloud_nlp_register(aui_t *aui, aui_nlp_cls_t *ops, aui_nlp_cb_t cb, void *priv);
int aui_cloud_tts_register(aui_t *aui, aui_tts_cls_t *ops, aui_tts_cb_t cb, void *priv);
int aui_cloud_asr_unregister(aui_t *aui);
int aui_cloud_nlp_unregister(aui_t *aui);
int aui_cloud_tts_unregister(aui_t *aui);
```

- 参数
  - aui: `aui_t`指针
  - ops： asr、nlp、tts的实现
  - cb：   asr、nlp、tts的事件回调
  - priv：asr、nlp、tts的用户参数

- 返回值
  - 0: 成功
  - < 0: 失败



```c
typedef void (*aui_asr_cb_t)(void *data, int len, void* asr_priv);
typedef void (*aui_nlp_cb_t)(void *data, int len, void* nlp_priv);
typedef void (*aui_tts_cb_t)(void *data, int len, void* tts_priv);
```

- data: 各项对应服务的data，默认为json数据包
- len：数据长度
- xxx_priv：asr、nlp、tts的用户参数



### 启动语音数据交互

```C
int aui_cloud_start_audio(aui_t *aui,int type);
```

* 参数
  * aui: `aui_t`指针
  * type： 1：进行asr服务，2：进行关键词识别服务
* 返回值
  * 0: 成功
  * < 0: 失败



### 向AUI系统输入语音数据

```C
int aui_cloud_push_audio(aui_t *aui, void *data, size_t size);
```

* 参数
  * aui: `aui_t`指针
  * data: 数据指针
  * size: 数据大小
* 返回值
  * 0: 成功
  * < 0: 失败

### 结束语音数据输入，AUI系统开始处理

```C
int aui_cloud_stop_audio(aui_t *aui);
```

* 参数
  * aui: `aui_t`指针
* 返回值
  * 0: 成功
  * < 0: 失败



### 启动nlp系统，连接云端

```C
int aui_cloud_start_nlp(aui_t *aui);
```

* 参数
  * aui: `aui_t`指针
* 返回值
  * 0: 成功
  * < 0: 失败

### 向AUI系统输入文本数据，AUI进行处理并回调NLP处理回调函数

```C
int aui_cloud_push_text(aui_t *aui, char *text);
```

* 参数
  * aui: `aui_t`指针
  * text: 文本字符串
* 返回值
  * 0: 成功
  * < 0: 失败





### 停止nlp系统，断开云端连接

```C
int aui_cloud_stop_nlp(aui_t *aui);
```

* 参数
  * aui: `aui_t`指针
* 返回值
  * 0: 成功
  * < 0: 失败



### 启动tts系统，连接云端

```C
int aui_cloud_start_tts(aui_t *aui);
```

* 参数
  * aui: `aui_t`指针
* 返回值
  * 0: 成功
  * < 0: 失败

### 向AUI系统输入文本数据，要求返回文本的TTS转换后的语音结果

```C
int aui_cloud_req_tts(aui_t *aui, const char *text);
```

* 参数
  * aui: `aui_t`指针
  * text: 需要转换的文本
* 返回值
  * 0: 成功
  * < 0: 失败

### 停止tts系统，断开云端连接

```C
int aui_cloud_stop_tts(aui_t *aui);
```

* 参数
  * aui: `aui_t`指针
* 返回值
  * 0: 成功
  * < 0: 失败

### 停止上云流程

```C
int aui_cloud_stop(aui_t *aui);
```

* 参数
  * aui: `aui_t`指针
* 返回值
  * 0: 成功
  * < 0: 失败


# 示例代码
```C
/* ai engine */
aui_t *g_aui_handler;

static void mit_tts_handle(void *data, int data_len, void *priv)
{
    char *json_text = (char *)data;
    LOGI("asr", "json= %s", json_text);
    
    ...  
}

static void mit_asr_handle(void *data, int len, void *priv)
{
    char *json_text = (char *)data;
    LOGI("asr", "json= %s", json_text);
    
    ...
}

/* ai engine init */
static int app_aui_nlp_init(void)
{
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

    aui_asr_register_mit(g_aui_handler, mit_asr_handle, NULL);
    aui_tts_register_mit(g_aui_handler, mit_tts_handle, NULL);

    return ret;
}
```

## 诊断错误码

无。

## 运行资源

无。

## 依赖资源

minilibc: v7.4.0及以上。
aos: v7.4.0及以上。

## 组件参考

无。