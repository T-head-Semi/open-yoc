/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __NLS_NUI_THINGS_TYPE_H__
#define __NLS_NUI_THINGS_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "nui_things_common_event.h"


typedef enum {
	kNlsNuiThingsDialogTypeVA,
	kNlsNuiThingsDialogTypeComb,
	kNlsNuiThingsDialogTypeDiv
}NlsNuiThingsDialogType;

typedef enum {
	kNlsNuiThingsStateInvalid =0,
	kNlsNuiThingsStateIdle,
	//kNlsNuiThingsStateConneton,//服务端不建议保持长链接
	kNlsNuiThingsStateAsrstarting,
	kNlsNuiThingsStateAsrupdating,
	kNlsNuiThingsStateAsrresultwaiting,
	kNlsNuiThingsStateDialogresultwaiting
} NlsNuiThingsState;

typedef enum {
	kNlsNuiThingsAudioTypeKws,
	kNlsNuiThingsAudioTypeAsr,
	kNlsNuiThingsAudioTypeVpr
}NlsNuiThingsAudioType;

typedef enum {
	kNlsNuiThingsEventStart,
	kNlsNuiThingsEventStop,
	kNlsNuiThingsEventVadStart,
	kNlsNuiThingsEventVadTimeout,
	kNlsNuiThingsEventVadEnd,
	kNlsNuiThingsEventWuw,
	kNlsNuiThingsEventVprResult,
	kNlsNuiThingsEventAsrPartialResult,
	kNlsNuiThingsEventAsrResult,
	kNlsNuiThingsEventAsrError,
	kNlsNuiThingsEventDialogResult,
	kNlsNuiThingsEventSessionStart,
	kNlsNuiThingsEventError,/*,
	
	kNlsNuiThingsEventTTSStart,
	kNlsNuiThingsEventTTSData,
	kNlsNuiThingsEventTTSFinish,
	kNlsNuiThingsEventTTSError*/
	kNlsNuiThingsEventOther,
}NlsNuiThingsEvent;
	
typedef int (*NlsNuiThingsEventCallback)(void * user_data, NlsNuiThingsEvent event, const char *event_response, const int event_length);

typedef struct {
	NlsNuiThingsEventCallback event_call_back;
	void * user_data;
}NlsNuiThingsListener;

/* TTS 模块回调事件 */
typedef enum {	
	kNlsNuiThingsTTSEventTTSStart,// TTS started
	kNlsNuiThingsTTSEventTTSData, // TTS voice data
	kNlsNuiThingsTTSEventTTSEnd,  // TTS finished. the end.
	kNlsNuiThingsTTSEventTTSError // TTS error. the end.
}NlsNuiThingsTTSEvent;

/* TTS 模块回调函数类型 */	
typedef int (*NlsNuiThingsTTSEventCallback)(void * user_data, NlsNuiThingsTTSEvent event, const char *event_response, const int event_length);

/* TTS 模块初始化时传入的回调参数类型 */	
typedef struct {
	NlsNuiThingsTTSEventCallback event_call_back; //回调函数
	void * user_data;                             //用户传入的私人数据，回调中会带有此参数。
	
	FunNuiThingsCommonEvent common_event_callback;//SDK通用功能事件回调函数
}NlsNuiThingsTTSListener;

#ifdef __cplusplus
}
#endif

#endif //__NLS_NUI_THINGS_TYPE_H__
