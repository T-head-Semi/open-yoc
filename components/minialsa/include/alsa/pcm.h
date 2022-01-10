/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <unistd.h>
#include <aos/kernel.h>

#ifndef __AOS_PCM__
#define __AOS_PCM__

#include <aos/ringbuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

//提供管理数字音频的捕捉和回放
#define PCM_EVT_WRITE (1 << 0)
#define PCM_EVT_READ (1 << 1)
#define PCM_EVT_XRUN (1 << 2)/** Stopped: underrun (playback) or overrun (capture) detected */

/** Non blocking mode (flag for open mode) \hideinitializer */
#define AOS_PCM_NONBLOCK        0x00000001
/** Async notification (flag for open mode) \hideinitializer */
#define AOS_PCM_ASYNC           0x00000002

typedef enum _aos_pcm_state {
	/** Open */
	AOS_PCM_STATE_OPEN = 0,
	/** Setup installed */
	AOS_PCM_STATE_SETUP,
	/** Ready to start */
	AOS_PCM_STATE_PREPARED,
	/** Running */
	AOS_PCM_STATE_RUNNING,
	/** Stopped: underrun (playback) or overrun (capture) detected */
	AOS_PCM_STATE_XRUN,
	/** Draining: running (playback) or stopped (capture) */
	AOS_PCM_STATE_DRAINING,
	/** Paused */
	AOS_PCM_STATE_PAUSED,
	/** Hardware is suspended */
	AOS_PCM_STATE_SUSPENDED,
	/** Hardware is disconnected */
	AOS_PCM_STATE_DISCONNECTED,
	AOS_PCM_STATE_LAST = AOS_PCM_STATE_DISCONNECTED,
	/** Private - used internally in the library - do not use*/
	AOS_PCM_STATE_PRIVATE1 = 1024
} aos_pcm_state_t;

typedef enum _aos_pcm_stream {
    AOS_PCM_STREAM_PLAYBACK = 0,
    AOS_PCM_STREAM_CAPTURE,
    AOS_PCM_STREAM_LAST = AOS_PCM_STREAM_CAPTURE,
} aos_pcm_stream_t;

typedef enum _aos_pcm_access {
    /** aos_pcm_readi/aos_pcm_writei access */
    AOS_PCM_ACCESS_RW_INTERLEAVED,
    /** aos_pcm_readn/aos_pcm_writen access */
    AOS_PCM_ACCESS_RW_NONINTERLEAVED,
    AOS_PCM_ACCESS_LAST = AOS_PCM_ACCESS_RW_NONINTERLEAVED
} aos_pcm_access_t;


typedef int aos_pcm_uframes_t;
typedef int aos_pcm_sframes_t;

typedef struct aos_pcm_hw_params {
    int access;
    int format;
    int subformat;
    int sample_bits;
    int frame_bits;
    int channels;
    int rate;
    int period_time;
    int period_size;
    int period_bytes;
    int periods;
    int buffer_time;
    int buffer_size;
    int buffer_bytes;
    int tick_time;
} aos_pcm_hw_params_t;

typedef struct aos_pcm_sw_params {
    int tstamp_mode;
    unsigned int period_step;
    unsigned int sleep_min;
    aos_pcm_uframes_t avail_min;
    aos_pcm_uframes_t xfer_align;
    aos_pcm_uframes_t start_threshold;
    aos_pcm_uframes_t stop_threshold;
    aos_pcm_uframes_t silence_threshold;
    aos_pcm_uframes_t silence_size;
    aos_pcm_uframes_t boundary;
    unsigned int tstamp_type;
    int pads;
    unsigned char reserved[52];
    unsigned int period_event;
} aos_pcm_sw_params_t;

typedef struct _aos_pcm aos_pcm_t;

typedef struct aos_pcm_ops {
    int (*open)(aos_pcm_t *pcm);
    int (*hw_params_set)(aos_pcm_t *pcm, struct aos_pcm_hw_params *params);
    int (*pause)(aos_pcm_t *pcm, int enable);
    int (*write)(aos_pcm_t *pcm, void *buf, int size);
    int (*read)(aos_pcm_t *pcm, void *buf, int size);
    int (*close)(aos_pcm_t *pcm);
    int (*wait)(aos_pcm_t *pcm, int timeout);
    int (*flush)(aos_pcm_t *pcm);
} aos_pcm_ops_t;

struct _aos_pcm {
    char *name;
    void *hdl;
    aos_pcm_state_t state;
    aos_pcm_stream_t stream;
    int mode;
    aos_pcm_hw_params_t *hw_params;
    aos_pcm_sw_params_t *sw_params;
    aos_mutex_t mutex;
    aos_pcm_ops_t *ops;
};

typedef struct aos_pcm_dev {
    aos_pcm_t pcm;
} aos_pcm_dev_t;

int aos_pcm_new(aos_pcm_t **pcm, int type, const char *name, aos_pcm_stream_t stream, int mode);
void aos_pcm_set_ops(aos_pcm_t *pcm, int direction, struct aos_pcm_ops *ops);

int aos_pcm_hw_params_alloca(aos_pcm_hw_params_t **p);
int aos_pcm_sw_params_alloca(aos_pcm_sw_params_t **p);

int aos_pcm_open(aos_pcm_t **pcm, const char *name, aos_pcm_stream_t stream, int mode);
int aos_pcm_close(aos_pcm_t *pcm);

int aos_pcm_pause(aos_pcm_t *pcm, int enable);
int aos_pcm_drop(aos_pcm_t *pcm);
int aos_pcm_drain(aos_pcm_t *pcm);

int aos_pcm_hw_params_set_access(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_access_t _access);
int aos_pcm_hw_params_set_format(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, int val);
int aos_pcm_hw_params_set_buffer_size_near(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t *val);
int aos_pcm_hw_params_set_period_size_near(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t *val, int *dir);
int aos_pcm_hw_params_current(aos_pcm_t *pcm, aos_pcm_hw_params_t *params);
int aos_pcm_hw_free(aos_pcm_t *pcm);
int aos_pcm_hw_params_any(aos_pcm_t *pcm, aos_pcm_hw_params_t *params);
int aos_pcm_hw_params(aos_pcm_t *pcm, aos_pcm_hw_params_t *params);
int aos_pcm_prepare(aos_pcm_t *pcm);
int aos_pcm_wait(aos_pcm_t *pcm, int timeout);
int aos_pcm_recover(aos_pcm_t *pcm, int err, int silent);

int aos_pcm_hw_params_set_channels(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);
int aos_pcm_hw_params_set_rate(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);
int aos_pcm_hw_params_set_rate_near(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int *val, int *dir);
int aos_pcm_hw_params_set_rate_resample(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);
int aos_pcm_hw_params_set_export_buffer(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);
int aos_pcm_hw_params_set_period_wakeup(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);
int aos_pcm_hw_params_set_period_time(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);
int aos_pcm_hw_params_set_period_size(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t val, int dir);
int aos_pcm_hw_params_set_periods(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);
int aos_pcm_hw_params_set_buffer_time(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);
int aos_pcm_hw_params_set_buffer_size(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t val);
int aos_pcm_set_params(aos_pcm_t *pcm, int format, aos_pcm_access_t acc, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency);


int aos_pcm_sw_params_current(aos_pcm_t *pcm, aos_pcm_sw_params_t *params);
int aos_pcm_sw_params_set_tstamp_mode(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, int val);
int aos_pcm_sw_params_set_avail_min(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);
int aos_pcm_sw_params_set_period_event(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, int val);
int aos_pcm_sw_params_set_start_threshold(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);
int aos_pcm_sw_params_set_stop_threshold(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);
int aos_pcm_sw_params_set_silence_threshold(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);
int aos_pcm_sw_params_set_silence_size(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);
int aos_pcm_sw_params(aos_pcm_t *pcm, aos_pcm_sw_params_t *params);


aos_pcm_sframes_t aos_pcm_writei(aos_pcm_t *pcm, const void *buffer, aos_pcm_uframes_t size);
aos_pcm_sframes_t aos_pcm_readi(aos_pcm_t *pcm, void *buffer, aos_pcm_uframes_t size);
aos_pcm_sframes_t aos_pcm_writen(aos_pcm_t *pcm, void **bufs, aos_pcm_uframes_t size);
aos_pcm_sframes_t aos_pcm_readn(aos_pcm_t *pcm, void **bufs, aos_pcm_uframes_t size);


aos_pcm_sframes_t aos_pcm_bytes_to_frames(aos_pcm_t *pcm, ssize_t bytes);
ssize_t aos_pcm_frames_to_bytes(aos_pcm_t *pcm, aos_pcm_sframes_t frames);

#ifdef __cplusplus
}
#endif

#endif