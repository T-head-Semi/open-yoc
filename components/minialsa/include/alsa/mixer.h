/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>

#ifndef __AOS_MIXER__
#define __AOS_MIXER__

#include <alsa/pcm.h>
#include <alsa/snd.h>
//控制发送信号和控制声音大小
#ifdef __cplusplus
extern "C" {
#endif


/** Mixer simple element channel identifier */
typedef enum _aos_mixer_selem_channel_id {
    /** Unknown */
    AOS_MIXER_SCHN_UNKNOWN = -1,
    /** Front left */
    AOS_MIXER_SCHN_FRONT_LEFT = 0,
    /** Front right */
    AOS_MIXER_SCHN_FRONT_RIGHT,
    /** Rear left */
    AOS_MIXER_SCHN_REAR_LEFT,
    /** Rear right */
    AOS_MIXER_SCHN_REAR_RIGHT,
    /** Front center */
    AOS_MIXER_SCHN_FRONT_CENTER,
    /** Woofer */
    AOS_MIXER_SCHN_WOOFER,
    /** Side Left */
    AOS_MIXER_SCHN_SIDE_LEFT,
    /** Side Right */
    AOS_MIXER_SCHN_SIDE_RIGHT,
    /** Rear Center */
    AOS_MIXER_SCHN_REAR_CENTER,
    AOS_MIXER_SCHN_LAST = 31,
    /** Mono (Front left alias) */
    AOS_MIXER_SCHN_MONO = AOS_MIXER_SCHN_FRONT_LEFT
} aos_mixer_selem_channel_id_t;

enum aos_mixer_selem_regopt_abstract {
    AOS_MIXER_SABSTRACT_NONE = 0,
    AOS_MIXER_SABSTRACT_BASIC,
};

typedef struct aos_mixer_selem_regopt {
    int ver;
    enum aos_mixer_selem_regopt_abstract abstract;
    const char *device;
    aos_pcm_t *playback_pcm;
    aos_pcm_t *capture_pcm;
} aos_mixer_selem_regopt_t;

typedef struct elem_inter {
    int cur;
    int min;
    int max;
} elem_inter_t;

typedef struct aos_mixer aos_mixer_t;
typedef struct _aos_mixer_elem aos_mixer_elem_t;

struct aos_mixer {
    aos_mixer_elem_t *playback_elem;
    aos_mixer_elem_t *capture_elem;
};

struct _aos_mixer_elem {
    const char *name;
    elem_inter_t vol_l;
    elem_inter_t vol_r;
    aos_mixer_t *mixer;
    aos_mutex_t mutex;
    void *hdl;
    struct sm_elem_ops *ops;
};

typedef struct sm_elem_ops {
    int (*set_volume)(aos_mixer_elem_t *elem, int l_value, int r_value);
} sm_elem_ops_t;

int aos_mixer_open(aos_mixer_t **mixer, int mode);
int aos_mixer_close(aos_mixer_t *mixer);
int aos_mixer_selem_set_playback_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int value);
int aos_mixer_selem_set_capture_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int value);
int aos_mixer_selem_set_playback_volume_all(aos_mixer_elem_t *elem, int value);
int aos_mixer_selem_set_capture_volume_all(aos_mixer_elem_t *elem, int value);
int aos_mixer_selem_set_playback_volume_range(aos_mixer_elem_t *elem, int min, int max);
int aos_mixer_selem_set_capture_volume_range(aos_mixer_elem_t *elem, int min, int max);
int aos_mixer_selem_get_playback_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int *value);
int aos_mixer_load(aos_mixer_t *mixer);
int snd_elem_new(aos_mixer_elem_t **elem_ret, const char *name, sm_elem_ops_t *ops);
void aos_mixer_free(aos_mixer_t *mixer);
aos_mixer_elem_t *aos_mixer_first_elem(aos_mixer_t *mixer);
aos_mixer_elem_t *aos_mixer_last_elem(aos_mixer_t *mixer);
aos_mixer_elem_t *aos_mixer_elem_next(aos_mixer_elem_t *elem);
aos_mixer_elem_t *aos_mixer_elem_prev(aos_mixer_elem_t *elem);
int aos_mixer_attach(aos_mixer_t *mixer, const char *name);

#ifdef __cplusplus
}
#endif

#endif