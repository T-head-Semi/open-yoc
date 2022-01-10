/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _VOICE_H_
#define _VOICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <voice_def.h>
#include "dbus_knife/kdbus_utils.h"
#include "dbus_knife/kdbus_introspect.h"

typedef enum {
    VOICE_EVENT_SESSION_BEGIN,
    VOICE_EVENT_SESSION_END,
    VOICE_EVENT_VAD,
    VOICE_EVENT_ASR_BEGIN,
    VOICE_EVENT_ASR_CHANGED,
    VOICE_EVENT_ASR_END,
    VOICE_EVENT_NLP,
    VOICE_EVENT_TTS_BEGIN,
    VOICE_EVENT_TTS_END,
} voice_event_t;

typedef enum {
    VOICE_ASR_EVT,
    VOICE_SILENCE_EVT,
    VOICE_DATA_EVT
} voice_evt_id_t;

typedef enum {
    VOICE_PCM_DATA,
    VOICE_OPUS_DATA,
    VOICE_USER_DATA
} voice_encode_t;

typedef enum {
    VOICE_STATE_IDLE,
    VOICE_STATE_BUSY,
    VOICE_STATE_WAIT,
    VOICE_STATE_STOPPED
} voice_state_t;

typedef void (*voice_evt_t)(void *priv, voice_evt_id_t evt_id, void *data, int len);
typedef void (*voice_pcm_hook)(void *data, int len);
typedef void (*voice_load_hook)(void);


typedef struct __voice_ops {
    int cache_cts;                  // number of cts_ms to cache, if 0 no cache is used
    int (*init)(void *priv);
    int (*deinit)(void *priv);
    int (*start_alg_tsk)(void *priv, void (*fn)(void *), void *arg);
    int (*set_param)(void *priv, void *param);
    int (*get_kws_data)(void *priv, void **data, size_t *data_len);
    int (*mute)(void *priv, int mute_flag);
    int (*p2t)(void *priv, int mode);
    int (*aec)(void *priv, void *mic, void *ref, int ms, void **out);
    int (*vad)(void *priv, void *mic, void *ref, int ms, void **out, size_t *out_len);
    int (*kws)(void *priv, void *mic, void *ref, int ms, void **out);
    int (*asr)(void *priv, void *vad_data, int ms);
} voice_ops_t;


typedef struct voice {
    csi_kws_t      *kws;       /* offline alg handle */
    // aui_t          *aui;       /* online  alg handle */
    DBusConnection *conn;      /* DBus connection handle */
    int             watch_rfd;
    void           *watch;
    voice_state_t   state;
    void           *priv;

    voice_ops_t         *ops;

    voice_pcm_param_t       *mic_param;
    voice_pcm_param_t       *ref_param;
    voice_param_t            param;

    int                      fake_wakeup;
    int                      silence_tmout;
    int                      silence_start;
    volatile int             task_running;

    long long                aec_time_stamp;
    long long                vad_time_stamp;
    long long                asr_time_stamp;

    aos_sem_t                sem;
    dev_ringbuf_t            v_ringbuf;
} voice_t;


/**
 * @brief  voice init
 * @param  [in] cb          : voice event callback
 * @param  [in] priv        : argv back to cb
 * @return !NULL on success, NULL on failed
 */
voice_t *voice_init(voice_evt_t cb, void *priv);

/**
 * @brief  voice deinit
 * @param  [in] v          : voice handle
 */
void voice_deinit(voice_t *v);

/**
 * @brief  voice config
 * @param  [in] v          : voice handle
 * @param  [in] p          : voice param
 * @return 0 on success, -1 on failed
 */
int voice_config(voice_t *v, voice_param_t *p);

/**
 * @brief  voice add and config mic(ref)
 * @param  [in] v          : voice handle
 * @param  [in] p          : voice pcm param
 * @return 0 on success, -1 on failed
 */
int voice_add_mic(voice_t *v, voice_pcm_param_t *p);
int voice_add_ref(voice_t *v, voice_pcm_param_t *p);

/**
 * @brief  voice start
 * @param  [in] v          : voice handle
 * @return 0 on success, -1 on failed
 */
int voice_start(voice_t *v);

/**
 * @brief  voice stop
 * @param  [in] v          : voice handle
 * @return 0 on success, -1 on failed
 */
int voice_stop(voice_t *v);

/**
 * @brief  voice pcm backflow control
 * @param  [in] v          : voice handle
 * @param  [in] id         : voice backflow id
 * @param  [in] flag       : 1:open
 * @return 0 on success, -1 on failed
 */
int voice_backflow_control(voice_t *v, voice_backflow_id_t id, int flag);

/**
 * @brief  voice message send
 * @param  [in] v          : voice handle
 * @param  [in] cmd        : voice cmd tag
 * @param  [in] data       : data to send
 * @param  [in] len        : data len
 * @param  [in] sync       : synced send
 * @return 0 on success, -1 on failed
 */
int voice_msg_send(voice_t *v, int cmd, void *data, int len, int sync);

/**
 * @brief  voice push to talk
 * @param  [in] v          : voice handle
 * @param  [in] mode         : typedef enum { }voice_p2t_mode;
 * @return 0 on success, -1 on failed
 */
int voice_push2talk(voice_t *v, voice_p2t_mode mode);

/**
 * @brief  voice set pcm hook
 * @param  [in] v          : voice handle
 * @param  [in] hook       : pcm hook function;
 * @return 0 on success, -1 on failed
 */
int voice_set_pcm_hook(voice_t *v, voice_pcm_hook hook);

/**
 * @brief  voice pcm open(close)
 * @param  [in] timeout    : close audio time
 * @return 0 on success, -1 on failed
 */
int voice_mute(voice_t *v, int timeout);
int voice_unmute(voice_t *v);

/**
 * @brief  voice pcm sampling pause/resume
 * @param  [in] v          : voice handle
 * @return 0 on success, -1 on failed
 */
int voice_pcm_pause(voice_t *v);
int voice_pcm_resume(voice_t *v);

/**
 * @brief  voice pcm register user event
 * @param  [in] v          : voice handle
 * @param  [in] evt        : pcm event
 * @param  [in] priv       : user private data
 * @return 0 on success, -1 on failed
 */
int voice_pcm_evt_register(voice_t *v, voice_pcm_evt_t evt, void *priv);


int voice_set_state(voice_t *voice, voice_state_t state);
voice_state_t voice_get_state(voice_t *voice);

#ifdef __cplusplus
}
#endif

#endif