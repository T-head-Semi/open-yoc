/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __VOICE_HAL_H__
#define __VOICE_HAL_H__

#include <stddef.h>
#include <pthread.h>
#include <dbus/dbus.h>
#include <yoc/mic.h>
#include <voice_def.h>

#ifdef __cplusplus
extern "C" {
#endif






typedef struct ai_param {
    int   sample_bits;      /* 采样精度 默认16bit*/
    int   channels;         /* 预留  */
    int   rate;             /* 采样率 默认16K*/
    int   nsmode;           /* 去噪等级参数  0~3 非线性处理等级逐步加强，其他值无非线性处理 */
    int   aecmode;          /* 回音消除等级  0~3 非线性处理等级逐步加强，其他值无非线性处理 */
    int   vadmode;          /* VAD等级 0~3 等级逐步加强 */
    int   sentence_time_ms; /* 有语音断句时间 */
    int   noack_time_ms;    /* 无语音超时时间 */
    int   max_time_ms;      /* 唤醒后总超时时间 */
    void *ext_param1;       /* 预留 */
    void *ext_param2;       /* 预留 */
    int   vadswitch;        /* 0 关闭VAD，1 打开, 2 打开起点关闭尾点 */
    int   vadfilter;        /* VAD过滤器类型， 0 关闭过滤器， 1~3 不同过滤器类型 */
    int   vadkws_strategy;  /* VAD KWS策略 */
    vad_thresh_t vadthresh_kws;    /* KWS VAD阈值 */
    vad_thresh_t vadthresh_asr;    /* ASR VAD阈值 */ 
    int   wwv_enable;        /* 使能算法侧唤醒二次确认功能 */
} ai_alg_param_t;



#define    ALG_CMD_DEBUG_MODE    0x1000
#define    ALG_CMD_ASR_MODE      0x2000

#define    KWS_ID_WWV_MASK      0x10000



typedef void (*csi_kws_event_cb_t)(csi_kws_t *kws, csi_kws_event_t event, void *data, size_t size, void *cb_arg);

int csi_kws_open(csi_kws_t *kws, const char *name, csi_kws_event_cb_t cb, void *cb_arg);
int csi_kws_close(csi_kws_t *kws);
int csi_kws_pcm_config(csi_kws_t *kws, csi_pcm_format_t *format);
int csi_kws_alg_config(csi_kws_t *kws, void *config, size_t size);
int csi_kws_event_control(csi_kws_t *kws, csi_kws_event_t event, int en);
int csi_kws_start(csi_kws_t *kws);
int csi_kws_stop(csi_kws_t *kws);
csi_kws_state_t csi_kws_get_state(csi_kws_t *kws);
void csi_kws_set_state(csi_kws_t *kws, csi_kws_state_t state);

csi_vad_state_t csi_kws_get_vad_state(csi_kws_t *kws);
// void csi_kws_set_vad_state(csi_kws_t *kws, csi_vad_state_t state);

void csi_kws_set_wku_voice_state(int stat, int buf_delay, int hw_delay);

int csi_kws_get_kws_data(char **kws_data, int *data_len);
int csi_kws_get_tagged_vad_data(char **vad_data, int *data_len, short **frame_tag, int *finished);

int csi_kws_send_cmd(csi_kws_t *kws, int cmd, void *data, size_t size, int wait);

int csi_kws_write_pcm(csi_kws_t *kws, void *pcm_in_data, size_t pcm_in_len);

#ifdef __cplusplus
}
#endif

#endif /* __VOICE_HAL_H__ */
