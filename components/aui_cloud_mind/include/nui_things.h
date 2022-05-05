/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

/* maxiao.maxiao
   define the 'mutex' for diff platform/HW 
   such as:
*/

#ifndef __NUI_THINGS_H__
#define __NUI_THINGS_H__

#ifdef __cplusplus 
extern "C"
{ 
#endif

#include <stdio.h>
#include <stdlib.h>

#include "nui_things_common_event.h"
#include "nui_things_define.h"
#include "nui_things_config.h"
#include "mit_rtos.h"

/*  NuiThingsVoiceData 结构体，NUI Things SDK返回的语音数据。
	
	NuiThingsVoiceMode [mode]：数据类型包含： 唤醒词数据、 VAD切分数据。（只支持单通道）
	[buffer]: 数据buffer
	[len]   : buffer中数据长度
	[finish]: 是否是最后一帧数据。
*/
typedef enum {
	kNuiThingsVoiceModeKws = 0,// kws voice data
	kNuiThingsVoiceModeAsr = 1,// asr voice data
}NuiThingsVoiceMode;
	
typedef struct {
	NuiThingsVoiceMode mode;
	char * buffer;
	int len;
	int finish;//是否是最后一帧数据。
}NuiThingsVoiceData;

typedef enum {
	kNuiThingsEventNone=0,
	kNuiThingsEventWwv,
	kNuiThingsEventWwvStart,
	kNuiThingsEventWwvEnd,
	kNuiThingsEventWwvData,
	kNuiThingsEventWwvConfirm,
	kNuiThingsEventWwvReject,
	kNuiThingsEventVadStart,
	kNuiThingsEventVadTimeout,
	kNuiThingsEventVadEnd,
	kNuiThingsEventVprResult,
	kNuiThingsEventAsrPartialResult,
	kNuiThingsEventAsrResult,
	kNuiThingsEventAsrError,
	kNuiThingsEventDialogResult,
	kNuiThingsEventAttrResult,
	/*
	kNuiThingsEventTTSStart,
	kNuiThingsEventTTSData,
	kNuiThingsEventTTSFinish,
	kNuiThingsEventTTSError*/
	kNuiThingsEventEnd
} NuiThingsEvent;
/*nui_things_event_get_str() 获取回调事件的描述信息
  return : 成功，返回描述字符串信息首地址。
*/
const char * nui_things_event_get_str(NuiThingsEvent event);

/*外部提供语音数据时，此回调用于获取语音数据
 *user_data: 传入的用户私有数据
 *buffer： 数据空间首地址
 *len:要获取的数据长度，单位 字节。
 * return:
 *   成功：实际送入的数据长度，单位字节。
 *   失败：其他值
 */
 typedef struct {
	int reserved;
 }NuiThingsData;
typedef int (*FunNuiThingsProvideData)(void * user_data, char * buffer, int len);

/*返回实际字节数，如果实际字节数和data->length不同，则必须是最终一帧数据，即 data->finish=1*/
typedef int (*FunNuiThingsProvideDataNls)(void * user_data, NuiThingsVoiceData * data);

/* 此回调用于SDK内部输出语音数据给调用方。
 * 区分唤醒数据（唤醒成功后开始回调，）、ASR数据（唤醒后，检测到asr start后开始回调， asr end后停止回调）。
 **/
typedef int (*FunNuiThingsReturnData)(void * user_data, NuiThingsVoiceData *data);
/*事件回调。*/
typedef int (*FunNuiThingsListenerOnEvent)(void * user_data, NuiThingsEvent event, int dialog_finish);

/*
用户回调接口、用户私有数据
*/
typedef struct  {
  FunNuiThingsListenerOnEvent on_event_callback;
  FunNuiThingsProvideData need_data_callback;
  FunNuiThingsReturnData put_data_callback;
  FunNuiThingsProvideDataNls need_data_callback_wwv;
  FunNuiThingsProvideDataNls need_data_callback_nls;

  FunNuiThingsCommonEvent common_event_callback;
  void *user_data;
}NuiThingsListener;

/*NuiThingsInitConfigContext config of ''context' json*/
typedef struct {
	const char * name; //namespace, string
	const char * key;  //key, string
	const char * value;//value, string
}NuiThingsInitConfigContext;

int nui_things_set_context(NuiThingsInitConfigContext * context);

typedef struct {
	NuiThingsListener *listener;
	//NuiThingsInitConfigContext *context_info; //需要额外设置的信息。
	//int context_info_len;                     //context_info中内容个数。

	NuiThingsMode mode;
	int enable_fe; //是否使能前端信号处理算法
	int enable_kws;//是否使能唤醒检测模块
	int enable_vad;//是否使能VAD功能

	int log_level;
	int log_link_enable;//是否开启全链路统计log打印 

	char * key_id;     //业务方鉴权所需阿里云账号id
	char * key_secret; //业务方鉴权所需阿里云账号secret
	char * app_key;    //业务方鉴权所需appkey
	char * sdk_code;   //业务方鉴权所需售卖码sdk_code
} NuiThingsInitConfig;



int nui_things_init(NuiThingsInitConfig * config /*nui_things_listener * listener*/);
int nui_things_uninit();
int nui_things_start(NuiThingsConfig *config);
int nui_things_stop(int cancel);

typedef enum {
	kNuiThingsInfoTypeWwv,   //唤醒消息
	kNuiThingsInfoTypeAsr,   //识别结果信息
	kNuiThingsInfoTypeDialog //对话结果信息
}NuiThingsInfoType;

/*
 * 返回信息长度。
 * 成功，返回实际长度； 失败，返回负值。
*/
int nui_things_info_get_length(NuiThingsInfoType info_type);
/*
 *获取对应信息字符串。
 *buf_size是buf缓存区大小，获取的信息存放在buf中。
 *成功，返回0；失败，返回负值。
*/
int nui_things_info_get(NuiThingsInfoType info_type, char * buf, int buf_size);

void nui_things_info_print(NuiThingsInfoType info_type);



NuiThingsEvent nui_things_update_audio(const char * buffer, int len, char * buffer_out, int *len_out, MitRtosAudioResult * result_info);


#ifdef __cplusplus 
}
#endif

#endif
