/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */
#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
#ifndef __VOICE_TYPEDEF_DBUS_H__
#define __VOICE_TYPEDEF_DBUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VOICE_VERSION_STR "v0.1.0"

#define VOICE_DBUS_SERVER    "org.voice.server"
#define VOICE_DBUS_PATH      "/org/voice/path"
#define VOICE_DBUS_INTERFACE "org.voice.interface"

#define VOICE_DBUS_SIGNAL_SESSION_BEGIN   "sessionBegin"
#define VOICE_DBUS_SIGNAL_SESSION_END     "sessionEnd"
#define VOICE_DBUS_SIGNAL_SHORTCUT_CMD    "shortcutCMD"
#define VOICE_DBUS_SIGNAL_VAD             "VAD"
#define VOICE_DBUS_SIGNAL_ASR_BEGIN       "ASRBegin"
#define VOICE_DBUS_SIGNAL_ASR_CHANGED     "ASRChanged"
#define VOICE_DBUS_SIGNAL_ASR_END         "ASREnd"
#define VOICE_DBUS_SIGNAL_NLP             "NLP"
#define VOICE_DBUS_SIGNAL_TTS_BEGIN       "TTSBegin"
#define VOICE_DBUS_SIGNAL_TTS_END         "TTSEnd"
#define VOICE_DBUS_SIGNAL_NETWORK_ERR     "networkErr"
#define VOICE_DBUS_SIGNAL_CALL_MUSIC_RESP  "musicResponse"
#define VOICE_DBUS_SIGNAL_CALL_MUTE_CHANGE "muteChange"


#define VOICE_DBUS_METHOD_CALL_START       "start"
#define VOICE_DBUS_METHOD_CALL_STOP        "stop"
#define VOICE_DBUS_METHOD_CALL_GET_STATE   "getState"
#define VOICE_DBUS_METHOD_CALL_PCM_CONFIG  "PCMConfig"
#define VOICE_DBUS_METHOD_CALL_ALG_CONFIG  "ALGConfig"
#define VOICE_DBUS_METHOD_CALL_ASR         "ASR"
#define VOICE_DBUS_METHOD_CALL_NLP         "NLP"
#define VOICE_DBUS_METHOD_CALL_TTS         "TTS"
#define VOICE_DBUS_METHOD_CALL_PLAY        "play"
#define VOICE_DBUS_METHOD_CALL_ONLINE      "online"
#define VOICE_DBUS_METHOD_CALL_MUSIC_REQ   "musicRequest"
#define VOICE_DBUS_METHOD_CALL_MUTE_SET    "muteSet"
#define VOICE_DBUS_METHOD_CALL_MUTE_GET    "muteGet"

#ifdef __cplusplus
}
#endif

#endif /* __VOICE_TYPEDEF_DBUS_H__ */
#endif
