/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef __NUI_THINGS_CONFIG_H__
#define __NUI_THINGS_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif
/*只有前端/前端+云*/
typedef enum {
	kNuiThingsModeFe=0,//only FE-ALG
	kNuiThingsModeNls,//only NLS
	kNuiThingsModeVad, // FE+NLS, vad mode
	kNuiThingsModeP2t, // FE+NLS, push2talk mode
	kNuiThingsModeKws, // FE+NLS, kws mode. not supported now
}NuiThingsMode;

const char * nui_things_config_get_str(NuiThingsMode mode);

typedef struct {
	/*mode/enable_alg 确定了内部模块的使能与否和调用逻辑*/

	//NuiThingsMode mode;
	int enable_fe; //是否使能前端信号处理算法,not used
	int enable_kws;//是否使能唤醒检测模块,not used
	int enable_vad;//是否使能VAD功能,not used

	/*--启动对话时的必传参数 start--*/
	char *app_key;//应用ID，必填。
	char *token;  //应用token，必填。token和appkey一一对应
	char *url;    //服务地址，必填。
	
	int enable_wwv;//是否使能wwv功能
	
	char * session_id;//对话标志，必填，同样的ID表示同一轮对话
	/*--启动对话时的必传参数 end--*/

	char *kws_format;//唤醒词服务的数据格式，enable_wwv=1时必填
	char *kws_model; //唤醒词服务的模型名称，enable_wwv=1时必填
	char *kws_word;  //客户端检测到的唤醒词，enable_wwv=1时必填
	
	char *sr_format;//音频编码格式，可选，默认是pcm
	char *sr_model;//not used
	char *sr_customization_id;//语音识别服务的定制模型ID，可选,not used
	
	int sample_rate;//音频采样率，内部固定为16000
	
	int enable_vad_cloud;//内部固定为0

	
	char * device_uuid;//设备唯一ID，可选。
	
	int debug_level;//not used
	int enable_intermediate_result;//内部固定为1
	int enable_sentence_detection;//not used
	int enable_punctuation_prediction;//内部固定为0
	int enable_inverse_text_normalization;//内部固定为1
	int enable_voice_detection;//内部固定为0
	int enable_word_level_result;//not used
	//...
} NuiThingsConfig;

void nui_things_config_print(NuiThingsConfig * config);

NuiThingsConfig * nui_things_config_create();
int nui_things_config_destroy(NuiThingsConfig * config);
//int nui_things_config_set_param(NuiThingsConfig * config, const char *key, const char * value);

#ifdef __cplusplus
}
#endif

#endif //__NUI_THINGS_CONFIG_H__