/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __NLS_NUI_THINGS_TTS_H__
#define __NLS_NUI_THINGS_TTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "nls_nui_things_config.h"
//#include "nls_nui_things_type.h" /*在nls_nui_things_config.h中已经包含*/

/* 获取TTS回调事件的描述字符，用于log打印*/
const char * nls_nuithings_tts_event_get_str(NlsNuiThingsTTSEvent event);

/*初始化TTS模块
  参数：
  	listener： 回调相关配置参数。取值见nls_nui_things_type.h 中 NlsNuiThingsTTSListener 结构定义
    config： 服务相关配置参数。取值见 nls_nui_things_config.h 中 NlsNuiThingsTTSConfig 结构定义
  返回值：
  	成功返回0，失败返回负值。
*/
int nls_nui_things_tts_init(NlsNuiThingsTTSListener * listener, NlsNuiThingsTTSConfig* config);

/*网络建连。 
  参数：
 	 config: 只用到config中的token字段，设置网络访问令牌。通常设置NULL即可
  		config=NULL 或者config->token=NULL，则会默认使用init时配置的token(静态token或者动态token)
  返回值：
  	成功返回0，失败返回负值。
*/
int nls_nui_things_tts_connect(NlsNuiThingsTTSConfig* config);

/*启动一条文本的TTS服务。 
  参数：
 	 text: 待合成文本，文本内容必须采用UTF-8编码，长度不超过300个字符（英文字母之间需要添加空格）
  返回值：
  	成功返回0，失败返回负值。
*/
int nls_nui_things_tts_start(const char * text);

/*停止当前TTS服务。 
  参数：
 	 cancel: 是否取消后续消息事件。
 	 	0 正常退出，可以获取后续所有事件。
 	 	1 退出并放弃后续事件。
  返回值：
  	成功返回0，失败返回负值。
*/
int nls_nui_things_tts_stop(int cancel);

/*断开网络连接
  返回值：
  	成功返回0，失败返回负值。
*/
int nls_nui_things_tts_disconnect();

/*TTS模块反初始化
  返回值：
  	成功返回0，失败返回负值。
*/
int nls_nui_things_tts_uninit();

#ifdef __cplusplus
}
#endif

#endif //__NLS_NUI_THINGS_H__
