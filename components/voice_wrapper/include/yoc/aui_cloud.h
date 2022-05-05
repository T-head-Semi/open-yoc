/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _AUI_CLOUD_H_
#define _AUI_CLOUD_H_

#include <aos/aos.h>
#include <aos/list.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif
/*************************************************
 * AUI CLOUD系统API
 * 语音解析(云端)、NLP Json解析及处理
 *************************************************/
#define MUSIC_PREFIX "music:"
#define MUSIC_URL_NLP  "{\"music::url\" : \"%s\"}"

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

typedef enum aui_tts_state {
    AUI_TTS_INIT       = 1,
    AUI_TTS_PLAYING    = 2,
    AUI_TTS_FINISH     = 3,
    AUI_TTS_ERROR      = 4,
} aui_tts_state_e;

typedef enum aui_wwv_resut {
    AUI_KWS_REJECT = 1,
    AUI_KWS_CONFIRM
} aui_kws_resut_e;

typedef void (*aui_asr_cb_t)(void *data, int len, void* asr_priv);
typedef void (*aui_nlp_cb_t)(void *data, int len, void* nlp_priv);
typedef void (*aui_tts_cb_t)(void *data, int len, void* tts_priv);

typedef struct _aui aui_t;

#define aui_cmm_cls  \
    void *priv; \
    int (*init)(aui_t *aui); \
    int (*deinit)(aui_t *aui); \
    int (*start)(aui_t *aui); \
    int (*stop)(aui_t *aui);

typedef struct aui_asr_cls {
    aui_cmm_cls;
    int (*push_data)(aui_t *aui, void *data, size_t size);
    int (*stop_push_data)(aui_t *aui);
} aui_asr_cls_t;

typedef struct aui_nlp_cls {
    aui_cmm_cls;
    int (*push_text)(aui_t *aui, char *text);
} aui_nlp_cls_t;

typedef struct aui_tts_cls {
    aui_cmm_cls;
    int (*req_tts)(aui_t *aui, const char *text);
} aui_tts_cls_t;

typedef struct aui_ops {
    aui_asr_cls_t *asr;
    aui_nlp_cls_t *nlp;
    aui_tts_cls_t *tts;
} aui_ops_t;

typedef struct aui_cb  {
    aui_asr_cb_t asr_cb;
    aui_nlp_cb_t nlp_cb;
    aui_tts_cb_t tts_cb;
    void *asr_priv;
    void *nlp_priv;
    void *tts_priv;
} aui_cb_t;

struct _aui {
    aui_config_t config;
    aui_ops_t ops;
    int asr_type;
    aui_cb_t cb;
};

typedef struct aui_kws {
    int    asr_type;
    char  *data;
    size_t len;
} aui_kws_t;

/**
 * AUI系统初始化
 * 
 * @param aui aui对象指针
 * @param config aui配置
 * @return 0:成功
 */
aui_t *aui_cloud_init(aui_config_t *config);

/**
 * AUI系统去初始化
 * 
 * @param aui aui对象指针
 * @return 0:成功
 */
int aui_cloud_deinit(aui_t *aui);

/**
 * 停止上云流程
 * 
 * @param aui aui对象指针
 * @return 0:成功
 */
int aui_cloud_stop(aui_t *aui);

/**
 * AUI系统（取消）注册asr、nlp、tts
 * 
 * @param aui  aui对象指针
 * @param ops  asr、nlp、tts的实现
 * @param cb   asr、nlp、tts的事件回调
 * @param priv asr、nlp、tts的用户参数
 * @return 0:成功
 */
int aui_cloud_asr_register(aui_t *aui, aui_asr_cls_t *ops, aui_asr_cb_t cb, void *priv);
int aui_cloud_nlp_register(aui_t *aui, aui_nlp_cls_t *ops, aui_nlp_cb_t cb, void *priv);
int aui_cloud_tts_register(aui_t *aui, aui_tts_cls_t *ops, aui_tts_cb_t cb, void *priv);
int aui_cloud_asr_unregister(aui_t *aui);
int aui_cloud_nlp_unregister(aui_t *aui);
int aui_cloud_tts_unregister(aui_t *aui);

/**
 * 启动语音数据交互
 *
 * @param aui aui对象指针
 * @param type pcm数据类型
 * @return 0:成功
 */
int aui_cloud_start_audio(aui_t *aui,int type);

/**
 * 向AUI系统输入语音数据
 *
 * @param aui aui对象指针
 * @param data 数据指针
 * @param size 数据大小
 * @return 0:成功
 */
int aui_cloud_push_audio(aui_t *aui, void *data, size_t size);

/**
 * 结束语音数据输入，AUI系统开始处理
 * 
 * @param aui aui对象指针
 * @return 0:成功
 */
int aui_cloud_stop_audio(aui_t *aui);

/**
 * 启动nlp系统，连接云端
 *
 * @param aui aui对象指针
 * @return 0:成功
 */
int aui_cloud_start_nlp(aui_t *aui);

/**
 * 向AUI系统输入文本数据，AUI进行处理并回调NLP处理回调函数
 * 
 * @param aui aui对象指针
 * @param text 文本字符串
 * @return 0:成功
 */
int aui_cloud_push_text(aui_t *aui, char *text);

/**
 * 停止nlp系统，断开云端连接
 *
 * @param aui aui对象指针
 * @return 0:成功
 */
int aui_cloud_stop_nlp(aui_t *aui);

/**
 * 启动tts系统，连接云端
 *
 * @param aui aui对象指针
 * @return 0:成功
 */
int aui_cloud_start_tts(aui_t *aui);

/**
 * 向AUI系统输入文本数据，要求返回文本的TTS转换后的语音结果
 *
 * @param aui aui对象指针
 * @param text 需要转换的文本
 * @return 0:成功
 */
int aui_cloud_req_tts(aui_t *aui, const char *text);

/**
 * 停止tts系统，断开云端连接
 *
 * @param aui aui对象指针
 * @return 0:成功
 */
int aui_cloud_stop_tts(aui_t *aui);

/*************************************************
 * 命令执行器
 *
 *    AUI云端处理后返回NLP结果，调用命令执行器进行具体的操作
 *    如：音乐播放、或继续调用AUI进行其他功能解析
 *
 *************************************************/

#define AUI_CMD_PROC_NOMATCH -1        /* 无法匹配到可处理的技能 */
#define AUI_CMD_PROC_MATCH_NOACTION -2 /* 匹配到技能执行器，但意图不支持或解析有错误 */
#define AUI_CMD_PROC_NET_ABNORMAL -3   /* 网络异常 */
#define AUI_CMD_PROC_ERROR -10         /* 其他错误 */


/**
 * AUI系统注册asr、tts
 * 
 * @param aui  aui对象指针
 * @param cb   asr、tts的事件回调
 * @param priv asr、tts的用户参数
 * @return 0:成功
 */
void aui_asr_register(aui_t *aui, aui_asr_cb_t cb, void *priv);

void aui_tts_register(aui_t *aui, aui_tts_cb_t cb, void *priv);


#ifdef __cplusplus
}
#endif
#endif
