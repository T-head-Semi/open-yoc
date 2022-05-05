/**
 * Copyright (C) 2021 Alibaba.inc, All rights reserved.
 *
 * @file   vad_api.h
 * @brief  
 * @author @alibaba-inc.com
 * @date   2021/01/04
 */
#ifndef __AILABS__VAD_API_VAD_API_H__
#define __AILABS__VAD_API_VAD_API_H__

#if (!(defined VAD_CALL) || !(defined VAD_IMPORT_OR_EXPORT))
#    if defined __WIN32__ || defined _WIN32 || defined _WIN64
#       define VAD_CALL __stdcall
#       ifdef  XENGINE_IMPLEMENTION
#           define VAD_IMPORT_OR_EXPORT __declspec(dllexport)
#       else
#           define VAD_IMPORT_OR_EXPORT __declspec(dllimport)
#       endif
#    elif defined __ANDROID__
#       define VAD_CALL
#       define VAD_IMPORT_OR_EXPORT __attribute ((visibility("default")))
#    else
#       define VAD_CALL
#       define VAD_IMPORT_OR_EXPORT __attribute ((visibility("default")))
#    endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct api_vad;

/**
 * VAD回调函数原型
 * @param usr_ptr: 回调函数参数
 * @param status: 语音状态，其中1表示检测到语音，0表示没有检测到语音
 * @return 用户自定义
 */
typedef int (*VADResultHandler)(void *usr_ptr, int status);

typedef struct api_vad *(*vad_new_func)(const char *cfg_fn);
typedef int (*vad_delete_func)(struct api_vad *engine);
typedef int (*vad_start_func)(struct api_vad *engine, void *usr_ptr, VADResultHandler vad_func);
typedef int (*vad_feed_func)(struct api_vad *engine, const char *data, int size);
typedef int (*vad_stop_func)(struct api_vad *engine);
typedef int (*vad_reset_func)(struct api_vad *engine);
typedef int (*vad_setting_func)(struct api_vad *engine, const char *json_conf);
typedef int (*vad_version_func)(struct api_vad *engine);

/**
 * 创建VAD引擎
 * 同步接口
 * @param cfg_fn: VAD资源路径
 * @return VAD句柄
 */
VAD_IMPORT_OR_EXPORT struct api_vad *VAD_CALL vad_new(const char *cfg_fn);

/**
 * 销毁VAD引擎
 * 同步接口
 * @param engine: VAD句柄
 * @return 0表示销毁成功，其他表示销毁失败
 */
VAD_IMPORT_OR_EXPORT int VAD_CALL vad_delete(struct api_vad *engine);

/**
 * 运行VAD引擎
 * 同步接口
 * @param engine: VAD句柄
 * @param usr_ptr: VAD回调函数参数
 * @param vad_func: VAD回调函数
 * @return 0表示运行成功，-1表示VAD句柄为NULL导致运行失败
 */
VAD_IMPORT_OR_EXPORT int VAD_CALL vad_start(struct api_vad *engine, void *usr_ptr, VADResultHandler vad_func);

/**
 * 喂音频数据给VAD引擎
 * 同步接口
 * @param engine: VAD句柄
 * @param data: 单声道、16-bit、16000Hz的音频数据
 * @param size: 音频数据大小
 * @return 0表示feed成功，-1表示VAD句柄为NULL导致feed失败
 */
VAD_IMPORT_OR_EXPORT int VAD_CALL vad_feed(struct api_vad *engine, const char *data, int size);

/**
 * 终止VAD引擎
 * 同步接口
 * @param engine: VAD句柄
 * @return 0表示终止成功，-1表示VAD句柄为NULL导致终止失败
 */
VAD_IMPORT_OR_EXPORT int VAD_CALL vad_stop(struct api_vad *engine);

VAD_IMPORT_OR_EXPORT int VAD_CALL vad_reset(struct api_vad *engine);

/**
 * 设置VAD阈值
 * 同步接口
 * @param engine: VAD句柄
 * @param json_conf: VAD阈值json串，例如"{\"AdditionalPauseTime\" : 300}"
 * @return 0表示设置成功，-1表示VAD句柄为NULL导致设置失败
 */
VAD_IMPORT_OR_EXPORT int VAD_CALL vad_setting(struct api_vad *engine, const char *json_conf);

VAD_IMPORT_OR_EXPORT void VAD_CALL  vad_show_pop(struct api_vad *engine);

VAD_IMPORT_OR_EXPORT int VAD_CALL vad_version(struct api_vad *engine);

#ifdef __cplusplus
}
#endif
#endif

