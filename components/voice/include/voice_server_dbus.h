/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */
#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
#ifndef __VOICE_SERVER_H__
#define __VOICE_SERVER_H__

#include <stddef.h>
#include <pthread.h>

#include <voice.h>
#include <voice_typedef_dbus.h>


#ifdef __cplusplus
extern "C" {
#endif


// typedef struct voice {
//     csi_kws_t      *kws;       /* offline alg handle */
//     // aui_t          *aui;       /* online  alg handle */
//     DBusConnection *conn;      /* DBus connection handle */
//     int             watch_rfd;
//     void           *watch;
//     voice_state_t   state;
//     voice_ops_t    *ops;
//     volatile int    task_running;
//     voice_evt_t     cb;
//     void           *priv;
//     aos_sem_t       sem;

//     int             fake_wakeup;
//     int             silence_tmout;
//     int             silence_start;
//     long long       aec_time_stamp;
//     long long       vad_time_stamp;
//     long long       asr_time_stamp;
// } voice_t;


extern voice_t *g_voice; //TODO: optimize me


int voice_dbus_init(voice_t *voice);
void voice_dbus_deinit(voice_t *voice);
void voice_loop_run(voice_t *voice);



int voice_dbus_signal_session_begin(voice_t *voice, void *keyword, size_t size);
int voice_dbus_signal_session_end(voice_t *voice);
int voice_dbus_signal_shortcut_cmd(voice_t *voice, void *keyword, size_t size);

int voice_dbus_signal_vad(voice_t *voice);
int voice_dbus_signal_asr_begin(voice_t *voice);
int voice_dbus_signal_asr_changed(voice_t *voice, const char *text);
int voice_dbus_signal_asr_end(voice_t *voice, const char *text);
int voice_dbus_signal_tts_begin(voice_t *voice, const char *url);
int voice_dbus_signal_tts_end(voice_t *voice, const char *url);
int voice_dbus_signal_network_err(voice_t *voice);
int voice_dbus_signal_mute_change(voice_t *voice, int new_state);
int voice_dbus_signal_music_resp(voice_t *voice, const char *text);


#ifdef __cplusplus
}
#endif

#endif /* __VOICE_SERVER_H__ */
#endif