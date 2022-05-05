/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef _APP_PLAYER_H_
#define _APP_PLAYER_H_

#include <stdint.h>
#include <smart_audio.h>
/************
 * 本地音播放
 ************/
typedef enum local_voice_tag {
	LOCAL_AUDIO_NET_CFG_CONFIG,
	LOCAL_AUDIO_NET_CFG_CONN,
	LOCAL_AUDIO_NET_CFG_FAIL,
	LOCAL_AUDIO_NET_CFG_START,
	LOCAL_AUDIO_NET_CFG_SWITCH,
	LOCAL_AUDIO_NET_CFG_TIMEOUT,
	LOCAL_AUDIO_NET_FAIL,
	LOCAL_AUDIO_NET_SUCC,
	LOCAL_AUDIO_OK,
	LOCAL_AUDIO_PLAY_ERR,
	LOCAL_AUDIO_SORRY,
	LOCAL_AUDIO_SORRY2,
	LOCAL_AUDIO_STARTING,
	LOCAL_AUDIO_BT_CONNECT,
	LOCAL_AUDIO_BT_DISCONNECT,
	LOCAL_AUDIO_WAKEUP_COMING,
	LOCAL_AUDIO_WAKEUP_GO_AHEAD,
	LOCAL_AUDIO_WAKEUP_HELLO,
	LOCAL_AUDIO_WAKEUP_HI,
	LOCAL_AUDIO_WAKEUP_IM_HERE,
	LOCAL_AUDIO_END,
} local_voice_tag_t;

typedef struct local_voice {
	local_voice_tag_t tag;
	char *name;
} local_voice_t;

int  local_audio_play(local_voice_tag_t name);
int  local_wakeup_audio_play(local_voice_tag_t name);
void local_audio_finish_check();
int local_audio_stat_get();

/*************
 * 播放器
 ************/
int app_player_init(void);

/*************
 * PA控制
 ************/
/**
 * set speaker mute
 *
 * @param[in] mute 1: set mute 0:cancel mute
 */
void app_speaker_mute(int mute);

/**
 * init speaker
 */
void app_speaker_init(void);

#endif