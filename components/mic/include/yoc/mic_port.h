/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __MIC_PORT_H__
#define __MIC_PORT_H__

#include <aos/aos.h>
#include <ulog/ulog.h>
#include <yoc/mic.h>

#define MIC_EVENT_VAD_FLAG (1 << 0)
#define MIC_EVENT_KWS_FLAG (1 << 1)
#define MIC_EVENT_ALL_FLAG (MIC_EVENT_VAD_FLAG | MIC_EVENT_KWS_FLAG)

typedef struct mic_ops
{
    int (*init)(mic_t *mic, mic_event_t mic_event); /* 模块初始化 */
    int (*deinit)(mic_t *mic);                      /* 去初始化 */
    int (*start)(mic_t *mic);                       /* 启动模块 */
    int (*stop)(mic_t *mic);                        /* 停止模块 */

    int (*set_param)(mic_t *mic, mic_param_t *param); /* pcm参数设置 */
    int (*ai_ctl)(mic_t *mic, int cmd, void *param);  /* vad、aec等参数设置 */

    int (*event_control)(mic_t *mic, int flag); /*事件控制*/

    int (*audio_control)(mic_t *mic, int enable, int timeout); /* 输入到算法的音频控制 */
    int (*pcm_data_control)(mic_t *mic, int type, int enable); /* 数据回流开关 */

    int (*debug_control)(mic_t *mic, int level);         /* 调试使能开关 */
    int (*kws_wakeup)(mic_t *mic, int en, int vad_flag); /* 强制唤醒 */
} mic_ops_t;

int mic_ops_register(mic_ops_t *ops);

#endif
