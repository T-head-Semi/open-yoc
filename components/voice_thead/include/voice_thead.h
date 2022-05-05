/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef _VOICE_THEAD_H_
#define _VOICE_THEAD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VOICE_KWS_EVT,
    VOICE_SILENCE_EVT,
    VOICE_DATA_EVT,
    VOICE_INVALID_EVT
} voice_evt_id_t;

typedef enum {
    VOICE_STATE_IDLE,
    VOICE_STATE_BUSY
} voice_state_t;


typedef struct voice {
    void           *priv;
    voice_state_t   state;

    int             task_running;
    aos_task_t      plugin_task;

    aos_sem_t       sem;
    // dev_ringbuf_t     v_ringbuf;

    void           *alg_param;
} voice_t;

#ifdef __cplusplus
}
#endif

#endif