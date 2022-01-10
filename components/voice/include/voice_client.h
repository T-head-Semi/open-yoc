/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */
#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
#ifndef __VOICE_CLIENT_H__
#define __VOICE_CLIENT_H__

#include <stddef.h>
#include <pthread.h>
#include <voice_server_dbus.h>
#include <voice_hal.h>
#include "dbus_knife/kdbus_introspect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct voice_client voice_client_t;

typedef int (*start_func)(voice_client_t *client);
typedef int (*stop_func)(voice_client_t *client);
typedef const char *(*get_state_func)(voice_client_t *client);
typedef int (*pcm_config_func)(voice_client_t *client, csi_pcm_format_t *format);
typedef int (*alg_config_func)(voice_client_t *client, void *config, int size);
typedef int (*asr_func)(voice_client_t *client, char *pcm, int size);
typedef int (*nlp_func)(voice_client_t *client, const char *text);
typedef int (*tts_func)(voice_client_t *client, const char *text);

typedef int (*event_callback)(int event, void *data, int size);

typedef struct voice_client {
    start_func       start;
    stop_func        stop;
    get_state_func   get_state;
    pcm_config_func  pcm_config;
    alg_config_func  alg_config;
    asr_func         asr;
    nlp_func         nlp;
    tts_func         tts;
    event_callback   callback;
    pthread_t        pthread;
    DBusConnection  *conn;
    void            *priv;
} voice_client_t;

voice_client_t *voice_client_new(event_callback callback);
void voice_client_free(voice_client_t *client);


int voice_client_func_start(voice_client_t *client);
int voice_client_func_stop(voice_client_t *client);
const char *voice_client_func_get_state(voice_client_t *client);
int voice_client_func_pcm_config(voice_client_t *client, csi_pcm_format_t *format);
int voice_client_func_alg_config(voice_client_t *client, void *config, int size);
int voice_client_func_asr(voice_client_t *client, char *pcm, int size);
int voice_client_func_nlp(voice_client_t *client, const char *text);
int voice_client_func_tts(voice_client_t *client, const char *text);

#ifdef __cplusplus
}
#endif

#endif /* __VOICE_CLIENT_H__ */
#endif
