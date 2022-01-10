/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <aos/debug.h>

#include <aos/kernel.h>
#include <alsa/mixer.h>
#include <alsa/snd.h>

#define TAG "mixer"

#define ELEM_LOCK(elem)                                                                                \
    do {                                                                                           \
        if (!aos_mutex_is_valid(&elem->mutex))                                                    \
            aos_mutex_new(&elem->mutex);                                                          \
        aos_mutex_lock(&elem->mutex, AOS_WAIT_FOREVER);                                           \
    } while (0)

#define ELEM_UNLOCK(elem)                                                                              \
    do {                                                                                           \
        aos_mutex_unlock(&elem->mutex);                                                           \
    } while (0)


extern sm_elem_ops_t g_mixer_ops;

int aos_mixer_open(aos_mixer_t **mixer_ret, int mode)
{
    aos_mixer_t *mixer;
    aos_mixer_elem_t *elem;

    mixer = aos_zalloc(sizeof(aos_mixer_t));
    elem  = aos_zalloc(sizeof(aos_mixer_elem_t));

    if (!mixer || !elem) {
        if (mixer) free(mixer);
        if (elem) free(elem);
        return -1;
    }

    mixer->playback_elem = elem;
    elem->ops = &g_mixer_ops;
    *mixer_ret = mixer;
    return 0;
}

int aos_mixer_close(aos_mixer_t *mixer)
{
    free(mixer->playback_elem);
    free(mixer);

    return 0;
}

int aos_mixer_attach(aos_mixer_t *mixer, const char *card_name)
{

    return 0;
}

int snd_elem_new(aos_mixer_elem_t **elem_ret, const char *name, sm_elem_ops_t *ops)
{

    return 0;
}

int aos_mixer_selem_register(aos_mixer_t *mixer, struct aos_mixer_selem_regopt *options, void **classp)
{

    return 0;
}

int aos_mixer_load(aos_mixer_t *mixer)
{

    return 0;
}

aos_mixer_elem_t *aos_mixer_first_elem(aos_mixer_t *mixer)
{
    aos_check_return_val(mixer, NULL);
    return mixer->playback_elem;
}

aos_mixer_elem_t *aos_mixer_last_elem(aos_mixer_t *mixer)
{
    aos_check_return_val(mixer, NULL);

    return mixer->playback_elem;
}

aos_mixer_elem_t *aos_mixer_elem_next(aos_mixer_elem_t *elem_p)
{
    aos_check_return_val(elem_p, NULL);

    return elem_p;
}

aos_mixer_elem_t *aos_mixer_elem_prev(aos_mixer_elem_t *elem_p)
{
    aos_check_return_val(elem_p, NULL);

    return elem_p;
}

static int _aos_mixer_elem_volume(aos_mixer_elem_t *elem, int l_value, int r_value)
{
    ELEM_LOCK(elem);
    elem->ops->set_volume(elem, l_value, r_value);
    ELEM_UNLOCK(elem);
    return 0;
}

int aos_mixer_selem_set_playback_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int value)
{
    aos_check_return_einval(elem);

    int ret = -1;

    if(AOS_MIXER_SCHN_FRONT_LEFT == channel) {
        ret = _aos_mixer_elem_volume(elem, value, elem->vol_r.cur);
        if(ret == 0) {
            elem->vol_l.cur = value;
        }
    } else if(AOS_MIXER_SCHN_FRONT_RIGHT == channel) {
        ret = _aos_mixer_elem_volume(elem, elem->vol_l.cur, value);
        if(ret == 0) {
            elem->vol_r.cur = value;
        }
    }

    return ret;
}

int aos_mixer_selem_set_capture_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int value)
{
    aos_check_return_einval(elem);

    return 0;
}

int aos_mixer_selem_set_playback_volume_all(aos_mixer_elem_t *elem, int value)
{
    aos_check_return_einval(elem);


    int ret = _aos_mixer_elem_volume(elem, value, value);
    if(ret == 0) {
        elem->vol_l.cur = elem->vol_r.cur = value;
    }

    return ret;
}

int aos_mixer_selem_set_capture_volume_all(aos_mixer_elem_t *elem, int value)
{
    aos_check_return_einval(elem);

    return 0;
}

int aos_mixer_selem_get_playback_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int *value)
{
    aos_check_return_einval(elem && value);

    if(AOS_MIXER_SCHN_FRONT_LEFT == channel) {
        *value = elem->vol_l.cur;
    } else if(AOS_MIXER_SCHN_FRONT_RIGHT == channel) {
        *value = elem->vol_r.cur;
    }

    return 0;
}