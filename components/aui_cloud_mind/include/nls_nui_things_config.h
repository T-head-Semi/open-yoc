/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __NLS_NUI_THINGS_CONFIG_H__
#define __NLS_NUI_THINGS_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 1
#include "nls_nui_things_type.h"
//#include "nls_dialog_impl.h"
#else
typedef enum {
	kNlsNuiThingsDialogTypeVA,
	kNlsNuiThingsDialogTypeComb,
	kNlsNuiThingsDialogTypeDiv
}NlsNuiThingsDialogType;
#endif

typedef struct {
	//内部指针变量，只是指针，不会主动申请空间，因此不需要释放。
	NlsNuiThingsDialogType dialog_type;//default kNlsNuiThingsDialogTypeVA
	int protocal_type;//vs
	
	int enable_wuw;        //使能唤醒二次确认
	int enable_vpr;        //使能声纹认证
	int enable_gender;     //使能性别认证
	int enable_decoder_vad;//是否开启解码器vad，可选，默认是false

	char *kws_format;//
	char *sr_format;//
	int sample_rate;//
	
	char *kws_word;//
	char *kws_model;
	char *vpr_service_id;//声纹服务的业务ID，可选
	char *vpr_group_id;  //声纹服务的用户组ID，必填
	
	/*
	char *sr_model;
	char *sr_customization_id;
	*/
	//vocabulary map 语音识别服务的热词列表，可选
	char * sr_model_lm_id;//语音识别服务的定制语言模型ID，可选
	char * sr_model_am_id;//语音识别服务的定制声学模型ID，可选

	char * session_id;

	char * device_uuid;  //设备唯一ID，可选。
	char * ip_addr;      //设备传入的IP
	char * mac_addr;     //设备传入的MAC

	int enable_vad_cloud;//是否开启云端VAD语音检测，外部设置生效
	
	char *token;
	char *url;
	char *app_key;
	char *dialect;
	
	int enable_punctuation_prediction;//是否开启标点，可选，默认是0
	//int enable_voice_detection;       //是否启动语音检测，可选，默认是False 未使用20200330
	
	int debug_level;
	
	int enable_intermediate_result;//??
	//int enable_sentence_detection;//??
	int enable_inverse_text_normalization;//是否在后处理中执行ITN，可选，开启
	//int enable_word_level_result;//??
	int enable_disfluency;//是否开启顺滑，开启
	//...
	
	char * dialog_context;//对话附加信息。 可选
} NlsNuiThingsConfig;

///*delete at 20200827*/ void nls_nui_things_config_print(NlsNuiThingsConfig * config);

NlsNuiThingsConfig * nls_nui_things_config_create();
int nls_nui_things_config_destroy(NlsNuiThingsConfig * config);
///*delete at 20200827*/ int nls_nui_things_config_set_param(NlsNuiThingsConfig * config, const char *key, const char * value);

/* '*_default' set all members of 'config' to default*/
///*delete at 20200827*/ int nls_nui_things_config_default(NlsNuiThingsConfig * config);
/* '*_reset' set all members of 'config' to 0*/
int nls_nui_things_config_reset(NlsNuiThingsConfig * config);

/* TTS 模块初始化时传入的配置参数类型*/
typedef struct {
	//内部指针变量，会申请空间，因此需要内部维护空间的申请和释放
	int sample_rate;//音频采样率，默认是16000
	int volume;     //音量，范围是0~100，默认50
	int speech_rate;//语速，范围是-500~500，默认是0
	int pitch_rate; //语调，范围是-500~500，默认是0
	char *voice;    //发音人，默认是xiaoyun
	char *format;   //音频编码格式，默认是pcm。支持的格式：pcm、mp3
	
	char * text;    //进行转换的文本，用户不需要关心此字段。

	char * url;     //[必须设置]。服务地址。
					//弹外公有云外网访问，"    wss://nls-gateway.cn-shanghai.aliyuncs.com/ws/v1"
					//弹内访问，生产环境，"wss://nls-gateway-inner.aliyuncs.com/ws/v1"
	char * app_key; //[必须设置]。管控台创建的项目Appkey。
	char * token;   //服务访问令牌。根据服务发布位置，可以是固定token，也可以是动态token（需配合key_id/key_secret）。
	//char * secret;  //预留
	//char * id;      //预留
	/* 以上变量，只在SDK引擎内部使用。*/

	/* 以下变量，只在SDK外层使用，引擎内部不使用。*/
	int log_level;     //log等级设置。取值[0,4]，越大打印内容越少。
	
	int cache_enable;  //是否开启ttscache功能，开启后使用TTSCache的默认配置:kTTSCacheTypeFile/100/cfg_enable
	const char * path; //开启TTScache功能后，缓存路径。默认 "/fatfs0/"

	/* 为定制化服务所需的用户账号id/secret，用于获取访问令牌。如公有云用户，需要设置此信息*/
	/* 用户账号id/secret同时非空，则使用动态token流程。此时忽略 token字段 */
    const char * key_id;    //用户账户id。 
	const char * key_secret;//用户账户secret
	/*
	  key_id    key_secret    token          use
   	  非NULL       非NULL     whatever    dynamic_token,ignore 'token'
   	  非NULL        NULL      whatever    return error
   	   NULL      whatever    valid-token static_token, user 'token'
   	   NULL      whatever      NULL      return error
	*/
	char * sdk_code; //sdk_code info.向mind询问具体配置信息。

	/*reserved
	TTSCacheType type;    //缓存存储方式，只支持kTTSCacheTypeFile
	int size;             //limit of size. fileCount-File/MemorySize-RAM/MemorySize-Flash
	int cfg_enable;       //是否存在配置文件。*/	
} NlsNuiThingsTTSConfig;

/* 配置类型变量的操作函数。只有SDK内部使用。SDK外部用户不需要关心*/
void nls_nui_things_tts_config_print(NlsNuiThingsTTSConfig * config);
NlsNuiThingsTTSConfig * nls_nui_things_tts_config_create();
int nls_nui_things_tts_config_reset(NlsNuiThingsTTSConfig * config);
int nls_nui_things_tts_config_set_text(NlsNuiThingsTTSConfig * config,const char * text);
int nls_nui_things_tts_config_free_text(NlsNuiThingsTTSConfig * config);
int nls_nui_things_tts_config_destroy(NlsNuiThingsTTSConfig * config);

#ifdef __cplusplus
}
#endif

#endif //__NLS_NUI_THINGS_H__
