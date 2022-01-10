/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _VOICE_DEF_H_
#define _VOICE_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/pcm.h>



// typedef struct __voice voice_t;
typedef struct __voice_pcm voice_pcm_t;
typedef void (*voice_pcm_send)(void *priv, void *data, int len);


typedef enum {
    VOICE_MIC_DATA,
    VOICE_REF_DATA,
    VOICE_VAD_DATA,
    VOICE_AEC_DATA,
    VOICE_KWS_DATA,     // 唤醒词音频，用于云端确认

    VOCIE_BACKFLOW_DATA
} voice_backflow_id_t;

typedef enum {
    VOICE_VADSTAT_SILENCE = 0,
    VOICE_VADSTAT_VOICE,
    VOICE_VADSTAT_ASR,
    VOICE_VADSTAT_ASR_FINI,
    VOICE_VAD_PCM_DATA,
} voice_vad_state;

typedef enum {
    VOICE_P2T_MODE_STOP = 0,
    VOICE_P2T_MODE_VAD,
    VOICE_P2T_MODE_FORCE
} voice_p2t_mode;


typedef struct {
    int type;
    int seq;
    int flag;
    int len;
    char data[0];
} voice_data_t;

typedef struct {
    char            *pcm_name;
    unsigned int     rate;
    int              sample_bits;
    int              access;
    int              channles;
    int              channles_sum;
    int              period_bytes;
} voice_pcm_param_t;

typedef struct {
    int     cts_ms;
    int     ipc_mode;
    void   *ai_param;     // 算法侧参数配置
    int     ai_param_len;
    void   *shm_addr;
} voice_param_t;

typedef struct {
    aos_pcm_t               *hdl;
    char                    *name;
    voice_pcm_param_t       *param;
    void                    *data;
    int                      len;
} voice_capture_t;

typedef struct {
    uint8_t         flag;            /** flag for MESSAGE_SYNC and MESSAGE_ACK */
    uint16_t        command;         /** command id the service provide */
    void           *req_data;            /** message data */
    int             req_len;             /** message len */
    void           *resp_data;
    int             resp_len;
} voice_msg_t;


typedef struct mic_vad_buf {
    int vad_tags[10];
    int len;
    char data[0];
} mic_vad_buf_t;

typedef void (*voice_msg_evt_t)(void *priv, voice_msg_t *msg);


typedef struct {
    voice_data_t **pbuf;
    uint32_t length;
    uint32_t ridx;
    uint32_t widx;
} voice_ringbuf_t;


typedef enum {
    PCM_PRE_SEND,
    PCM_POST_SEND,
} pcm_evt_t;

typedef void (*voice_pcm_evt_t)(void *priv, pcm_evt_t pcm_evt, void *data, size_t len);


/* pcm format definition */
typedef struct {
    int channel_num;
    int sample_rate;
    int sample_bits;
    int period_time;
} csi_pcm_format_t;
/*************************/

/**************************************/

typedef enum {
    KWS_STATE_IDLE,             /* 空闲 */
    KWS_STATE_BUSY,             /* 唤醒 */
    KWS_STATE_WAIT,             /* 等待结束 */
    KWS_STATE_STOPPED,          /* 停止 */
    KWS_STATE_ASR,              /* ASR态 */
} csi_kws_state_t;

typedef enum {
    VAD_STATE_INVALID    = -1,   // VAD检测结果无变化，即保持安静或者有声音
    VAD_STATE_STARTPOINT,        // 检测到起点消息
    VAD_STATE_ENDPOINT,          // 检测到尾点消息
    VAD_STATE_TIMEOUT            // 检测到静音超时消失
}csi_vad_state_t;



/* voice handle definition */
typedef struct {
    char             *name;
    csi_pcm_format_t *format;
    void             *config;
    void             *cb;
    void             *cb_arg;
    int               event_en;
    csi_kws_state_t   state;
    csi_vad_state_t   vad_state;
    void             *priv;
} csi_kws_t;
/***************************/

typedef enum {
    KWS_EVENT_SESSION_BEGIN  = 0, /* 会话开始事件，“天猫精灵”识别后上报 */
    KWS_EVENT_SESSION_END,   /* 会话结束事件，语音断句时间超过一定阈值后上报 */
    KWS_EVENT_VAD,           /* 识别出来是人声则上报，按照算法帧时间上报，例如30ms */

    KWS_EVENT_PCM_ORIGINAL,  /* 原始数据，按照算法帧上报 */
    KWS_EVENT_PCM_ALG,       /* 算法后数据，按照算法帧上报 */
    KWS_EVENT_PCM_KWS,       /* KWS识别成功的数据，例如“天猫精灵”四个字的语音数据 */

    KWS_EVENT_AI_ALG         = 0x10,  /* AI算法自定义事件 */
    KWS_EVENT_SHORTCUT_CMD   /* 快捷命令词结果 */
} csi_kws_event_t;

// void *voice_malloc(unsigned int size);
// void voice_free(void *data);
// void voice_rb_create(voice_ringbuf_t *rb, size_t size);
// void voice_rb_destroy(voice_ringbuf_t *rb);
// voice_data_t *voice_rb_get(voice_ringbuf_t *rb);
// int voice_rb_put(voice_ringbuf_t *rb, const char *data, size_t len);

voice_pcm_t *pcm_init(voice_pcm_send send, void *priv);
void pcm_deinit(voice_pcm_t *p);
void pcm_mic_config(voice_pcm_t *p, voice_pcm_param_t *param);
void pcm_ref_config(voice_pcm_t *p, voice_pcm_param_t *param);
int pcm_start(voice_pcm_t *p);
int pcm_pause(voice_pcm_t *p);
int pcm_resume(voice_pcm_t *p);
int pcm_evt_register(voice_pcm_t *p, voice_pcm_evt_t evt, void *priv);


#ifdef __cplusplus
}
#endif

#endif
