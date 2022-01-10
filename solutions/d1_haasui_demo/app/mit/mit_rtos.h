/* maxiao.maxiao
   define the interface and structure for MIT_RTOS
*/


#ifndef __MIT_RTOS_H__
#define __MIT_RTOS_H__

#ifdef __cplusplus 
extern "C"
{ 
#endif

#include <stdio.h>
#include <stdlib.h>

typedef enum {
	kMitRtosRetSuccess = 0,

	kMitRtosRetKwsInitError = 20041,
}MitRtosRetValue;

/*定义MIT_RTOS模块返回的事件类型*/
typedef enum {
	MIT_RTOS_EVENT_NONE=0,
	MIT_RTOS_EVENT_WWV,
	MIT_RTOS_EVENT_WWVSTART,
	MIT_RTOS_EVENT_WWVEND,
	MIT_RTOS_EVENT_WWVDATA,
	MIT_RTOS_EVENT_WWVCONFIRM,
	MIT_RTOS_EVENT_WWVREJECT,
	MIT_RTOS_EVENT_VADSTART,
	MIT_RTOS_EVENT_VADTIMEOUT,
	MIT_RTOS_EVENT_VADEND,
	/*MIT_RTOS_EVENT_VPRRESULT,
	MIT_RTOS_EVENT_ASRPARTIALRESULT,
	MIT_RTOS_EVENT_ASRRESULT,
	MIT_RTOS_EVENT_ASRRERROR,
	MIT_RTOS_EVENT_DIALOGRESULT,*/
} mit_rtos_event_t;


//return the length got 需要注册，但未实际使用
typedef int (*MitRtosDataGet)(void * user_data, char * buffer, int len);
typedef int (*MitRtosDataOut)(void * user_data,  mit_rtos_event_t event, char * buffer, int len);
typedef int (*MitRtosEvent)(void * user_data, mit_rtos_event_t event, int dialog_finish);


/*定义MIT_RTOS模块 回调相关参数。 需要注册，但未实际使用*/
typedef struct  {
  MitRtosEvent on_event_callback;   //SDK返回事件的回调接口
  MitRtosDataGet need_data_callback;//SDK获取数据的回调接口
  MitRtosDataOut put_data_callback; //
  void *user_data;                  //用户私有数据，作为回调函数参数
}mit_rtos_listener;

/*定义MIT_RTOS模块 信号处理算法选择*/
typedef enum {
	kMitRtosAlgBypass = 0,
	kMitRtosAlg1MicFixed = 1,
	kMitRtosAlg2MicFloat = 2,         // pmwf算法，在用
	kMitRtosAlg2MicFixed = 3,
	kMitRtosAlg2MicBssFloat = 4,
	kMitRtosAlg2Mic1AecFloat = 5,
	kMitRtosAlg1Mic1AecFloat = 6,
	kMitRtosAlg2MicFloatThead2ed = 7,
	kMitRtosAlgBSS = 8,               //BSS算法，在用
	kMitRtosAlgPMWFPG = 9,
	kMitRtosAlgAECBSS = 10,           //AECBSS算法，在用
	kMitRtosAlgSweeper4mic = 11,      //4mic sweeper算法，在用
	kMitRtosAlgBSSOpt = 12,           //BSSOPT算法，在用
	kMitRtosAlgSweeper6mic = 13,      //6MIC6OUT算法，在用
}MitRtosAlgType;

typedef enum {
	kMitRtosVoiceFilterTypeDisable = 0, //关闭此功能
	kMitRtosVoiceFilterTypeDelPlay = 1, //删除播放过程语音
	kMitRtosVoiceFilterTypeDelLeng = 2, //删除固定长度
	kMitRtosVoiceFilterTypeVad     = 3  //通过VAD过滤
}MitRtosVoiceFilterType;

typedef struct {
	float vad_speech_noise_thres_sp;//起点阈值 取值范围 [-1, 1],精度一位小数
	float vad_speech_noise_thres_ep;//尾点阈值 取值范围 [-1, 1],精度一位小数
}MitRtosVadThres;

typedef struct {	
	int mic_num; //mic ch num. notusednow. default -1
	int ref_num; //ref ch num. must set for alg.6mic6out  and just use for alg.6mic6out. default 0(notusednow)
	int out_num; //mic ch num. notusednow. default -1
	
	int agc_enable;          //是否使能FE的AGC模块。0 关闭、1 开启。-1 使用算法内部默认值。[必须设置]
	int agc_power;           // AGC_Power. 取值范围(0,+无穷大)。设置0，使用默认值。[必须设置]
	int agc_level;           // AGC_Level. 取值范围(0,+无穷大)。设置0，使用默认值。[必须设置]
	//float agc_gain;          // FE的AGC模块 soft gain。单位倍数，取值范围(0,+无穷大)。设置0，使用默认值。[新版本中会统一到gain_tune中]
}MitRtosFeTune;

/* 多路选择策略 */
typedef enum {
	kMitRtosMcsWaitTime=0,//依赖固定长度
	kMitRtosMcsWaitEngine,//依赖引擎消息 [reserved]
}MitRtosMcsWait;

typedef enum {
	kMitRtosMcsCondSnr=0,       //依赖SNR
	kMitRtosMcsCondConfidence,  //依赖置信度
	kMitRtosMcsCondConfidenceCC,//依赖置信度CC [reserved]
	kMitRtosMcsCondEnergy,      //依赖能量         [reserved]
}MitRtosMcsCond;

/* 定义MIT_RTOS模块初始化配置信息*/
typedef struct {
	mit_rtos_listener * listener; //回调参数。默认NULL

	MitRtosAlgType alg_type; //模块算法选择。默认kMitRtosAlg2MicFloat 
	
	char task_enable ;       //use new task to run SDK. reserved. default 1
	char kws_alwayson_enable;//唤醒检测保持运行。识别状态也进行唤醒检测. default 1
	
	char fe_enable;          //是否使能AFE子模块. default 1
	char kws_enable;         //是否使能kws子模块. default 1
	char vad_enable;         //是否使能VAD子模块,for kws_state_handle. default 1
	char vad4asr_enable;     //是否使能ASR专用VAD子模块,enable_individual_vad4asr=1时有效. default 1
	
	char wwv_enable;         //是否使能唤醒二次确认联动功能	0不需要二次确认、1需要二次确认(相关门限使用算法预置参数). default 0
	short local_threshold;    //reserved 唤醒本地门限，    百分比 [0- 100]。引擎内部使用
	short wwv_threshold;      //reserved 唤醒二次确认门限，百分比[0-100]. 实际kws引擎内部未使用，外层封装时控制

	char need_data_after_vad;//process时是否需要外传语音数据给调用方。 如果不需要保存算法处理后数据，则可以设置为0 关闭此功能。默认1

	int vad_endpoint_delay;//单位ms。  默认200
	int vad_silencetimeout;//单位ms  。默认5000
	int vad_voicetimeout;  //单位ms  。默认10000
	//float vad_speech_noise_thres;//语音到安静的阈值,取值范围 [-1, 1],精度一位小数

	int vad_kws_strategy;     //VAD动态设置参数。 设置0 使用统一阈值，kws.thres_sp; 其他参数 预留
	MitRtosVadThres kws_thres;//唤醒模块的阈值。 
	MitRtosVadThres asr_thres;//识别模块的阈值。
	
	MitRtosVoiceFilterType voice_filter_type; //int 数值。默认DelLeng
	
	int vad_endpoint_ignore_enable;//开关变量0/1  。默认0
	int log_in_asr_enable;         //asr过程中，打印相关log信息。起尾点、数据结束、关键位置等信息，default 0. reserved
	//float gain_tune;             //调整软增益参数。 单位倍数，取值范围(0,+无穷大)。设置0，使用默认值。--//FE模块参数老配置方式，无法从.conf文件获取默认配置数值。升级FE模块后需要改为gain_tune_db
	float gain_tune_db;            //调整软增益参数。 单位db，取值范围(-75,+无穷大)。设置-100，使用默认值。
	MitRtosFeTune fe_tune;         //调整FE模块参数的结构体。

	int enable_individual_vad4asr; //enable individual vad for asr logic or not. 0 disable, 1 enable.default 1

	int enable_init_more_log;      //开启更多init过程中的log信息。 2时开启，其他值关闭。 default 2
	int is_interleave;             //voice type to sdk is uninterleaved(2) or interleaved(others-default 1). data_length must be 20ms per process
	int add_kwsdetect_enable;      //enalbe or not.default 0
	int opt_memory_level;          // memory opt level. 0 disable/maximal, 1 smallest, 2 smaller than normal, 3 normal = 0
} MitRtosConfig;

/*打印给定配置参数的相关信息。用于调试log查看
  参数：
 	 config: MIT_RTOS模块初始化配置参数
  返回值：
  	无
*/
void mit_rtos_config_print(MitRtosConfig * config, const char * tag);
/*设置参数为默认值
  参数：
 	 config: MIT_RTOS模块初始化配置参数
  返回值：
  	无
*/
void mit_rtos_config_default(MitRtosConfig * config);

/*定义VAD事件*/
typedef enum {
	kMitRotsVadResultInvalid = -1,//VAD检测结果无变化，即保持安静或者有声音
	kMitRotsVadResultStartPoint,  //检测到起点消息
	kMitRotsVadResultEndPoint,    //检测到尾点消息
	kMitRotsVadResultTimeout      //检测到静音超时消失
}MitRotsVadResult;

/*打印VAD事件的描述字符，用于log打印
  参数：
 	 vad_result: VAD事件
  返回值：
  	对应描述符信息。
*/
const char * mit_rtos_vadstate_get_string(MitRotsVadResult vad_result);

/*定义唤醒信息*/
typedef struct {
	int noise_energy;          //噪声能量
	int speech_energy;         //语音能量
	int snr;                   //信噪比
	float snr_pre_agc;         //AGC前的信噪比
	int snr_pre_agc_energe_kws;//AGC前的信噪比的kws部分语音能量
	int confidence;            //置信度
}MitRtosDataKwsInfo_t;

/*定义唤醒信息*/
typedef struct {
	int index;      //the index of all channels
	char * data;    //唤醒词切分数据缓存指针 [reserved]
	int length;     //唤醒词切分数据长度      [reserved]
	char word[24];  //唤醒词文本,中文汉字
	char word_pinyin[24];  //唤醒词文本，拼音
    float thresh_day_;
    float thresh_night_;
    float thresh_low_;
	int confidence; //唤醒词打分
	short cc_confidence;//reserved
	char do_wwv;    //本次唤醒是否需要进行二次确认验证。输出到用户使用
	//int energy;     //唤醒语音的能量信息
	MitRtosDataKwsInfo_t kws_info;
	int kws_spot_sp_offset;  //唤醒成功时 keyword起点到当前spot点的差异，单位ms
	int kws_spot_ep_offset;  //唤醒成功时 keyword尾点到当前spot点的差异，单位ms
}MitRtosDataKws;

/*定义MIT_RTOS模块传入的设备状态信息*/
typedef struct MitRtosDevState_{
	int available;  /* 数据是否有效 0 无效，内部不使用。 1 有效，内部使用 */
	int play_type;  /* 播放内容类型。0 无播放,1 播放事件提示音,2 播放TTS,3 播放内容资源(音乐、故事等资源). -1 标识无效*/
	int move_speed; /* 运动速度。 0 静置， 1 0<速度<=10km/h, 2 10km/h<速度<=20km/h, 3 ... 5 40km/h<速度<=50km/h. -1 标识无效*/
	int volume;     /* 播放音量 0<= volume <=100. -1 标识无效 */
} MitRtosDevState;


/*定义MIT_RTOS模块检测到的消息内容*/
typedef struct MitRtosAudioResult_{
	MitRotsVadResult vad_state;  //当前VAD状态
	int data_kws_count;          //检测到的唤醒词个数。0 未检测到唤醒词; >0,检测到多个唤醒词
	MitRtosDataKws data_kws[3];  //检测到唤醒词时的具体唤醒信息
	//int kws_bestend;             //检测到唤醒数据的最佳尾点，用于截取唤醒语音。 1找到bestend，0未找到bestend。
	                             //kws_bestend=1后，可以一次性获取所有唤醒数据。 
	MitRtosDevState dev_state;   //设备当前状态。
} MitRtosAudioResult;

/*初始化MIT_RTOS模块
  参数：
  	config 初始化配置参数
  返回值：
  	成功返回0，失败返回负值。
*/
int mit_rtos_init(MitRtosConfig * config);

/*反初始化MIT_RTOS模块
  参数：
  	无
  返回值：
  	成功返回0，失败返回负值。
*/
int mit_rtos_uninit();


/* MIT_RTOS模块启动模式
 * kMitRotsModeKws kws模式，会进行前端信号处理和kws检测、唤醒后进行ASR流程。
 * kMitRotsModeVad vad模式，会进行前端信号处理和vad检测，检测到vad结果后停止。检测到停止前也可以外部调用stop停止。
 * kMitRotsModeP2t push2talk模式，只进行前端信号处理，外部触发数据起始和停止时刻
 * kMitRotsModeLPM1st 刚从低功耗切回来的模式，只进行KWS模块处理。not support now 20200929
*/
typedef enum {
	kMitRotsModeKws = 0,
	kMitRotsModeVad,
	kMitRotsModeP2t,
	kMitRotsModeLPM1st,
	kMitRotsModeVadKws,
	kMitRotsModeP2tKws,
	kMitRotsModeMAX,   //invalid
}MitRotsMode;

const char * mit_rtos_start_mode_get_string(MitRotsMode mode);
/*  mit_rtos_start_mode(MitRotsMode mode) 启动MIT_RTOS模块
  参数：
  	mode 启动模式，
  返回值：
  	成功返回0，失败返回负值。
*/
int mit_rtos_start_mode(MitRotsMode mode);

/* mit_rtos_start() 启动MIT_RTOS模块,相当于启动模式为mit_rtos_start_mode(kMitRotsModeKws)
  参数：
  	无.
  返回值：
  	成功返回0，失败返回负值。
*/
int mit_rtos_start();


/*停止MIT_RTOS模块
  参数：
  	无
  返回值：
  	成功返回0，失败返回负值。
*/
int mit_rtos_stop();

/*向SDK输入语音数据，并获取算法处理后数据、事件消息。
  参数：
  	buffer：      输入语音数据地址
  	len：         'buffer'中数据长度，单位字节。要求每次数据长度是20ms，即2mic+1ref数据来说，是320*3ch*2=1920字节。
  	buffer_out： 算法处理后数据保存地址
  	len_out：     作为输入标识'buffer_out'的空间大小。 作为输出，标识算法实际写入'buffer_out'的数据长度，必须对应20ms的输出通道数量，即320*out_ch_num*2。 单位字节。 输出长度过小会直接返回错误码
  	result_info：返回MIT_RTOS模块内的消息
  返回值：
  	失败返回负值
  	唤醒未成功，返回0
  	唤醒成功，返回0x01. 找到唤醒数据最佳尾点，返回0x02. 唤醒成功并同时得到bestend，则返回3(0x01|0x02). 4...10reserved
  	命中命令词，则返回正数（>=11）,无bestend概念。
  	if (ret == 0) {
		//not wake up
  	} else if (ret>0) {
  		//唤醒相关消息 主唤醒词唤醒成功/命令词成功/得到唤醒数据尾点
  		if (ret > 10) {
			//命令词命中
	  	} else {
	  		if (ret &0x01) {
				//主唤醒词唤醒成功。可以获取唤醒附加消息，但二次确认数据还未准备完毕
	  		}
	  		if (ret &0x02) {
				//主唤醒词二次确认数据准备完毕
	  		}

	  	}
  	} else {
		//出错
  	}
*/
mit_rtos_event_t mit_rtos_update_audio(const char * buffer, int len, char * buffer_out, int *len_out, MitRtosAudioResult * result_info);
/*mit_rtos_update_audio()      would ignore info of result_info->dev_state.
 *mit_rtos_update_audio_info() would parse  info of result_info->dev_state.
 */
mit_rtos_event_t mit_rtos_update_audio_info(const char * buffer, int len, char * buffer_out, int *len_out, MitRtosAudioResult * result_info);

/* reserved */int mit_rtos_get_audio(char * buffer_out, int length);
/* reserved */int mit_rtos_event_get_kws(char * buffer);

#define MIT_RTOS_VOICE_DATA_LENGTH_MS 20
#define MIT_RTOS_VOICE_DATA_LENGTH_BYTE (MIT_RTOS_VOICE_DATA_LENGTH_MS*16*2)

/*定义需要获取语音的内容。是KWS语音还是ASR语音*/
typedef enum {
	kMitRtosVoiceModeKws = 0,// kws voice data
	kMitRtosVoiceModeAsr = 1,// asr voice data
}MitRtosVoiceMode;


/*[reserved] define the state of voice_data get from mit_rtos*/
typedef enum {
	voice_data_flag_disable = 0,// data not available
	voice_data_flag_start ,// start of voice_section
	voice_data_flag_available,// available of voice_section
	voice_data_flag_end,	  // end of voice_section
	voice_data_flag_timeout,  // special for vad timeout
	voice_data_flag_error,	  // error of voice_section
}mit_rtos_voice_data_flag;

/*定义获取的语音是否有效*/
typedef enum {
	kVoiceDataInvalid	= 0,// invalid
	kVoiceDataAvailable = 1,// available
}MitRtosVoiceDataAvailable;

typedef struct {
	MitRtosVoiceMode mode;               //获取什么数据，kws/asr [in]
	MitRtosVoiceDataAvailable available; //本次获取的数据是否有效 [out]
	//mit_rtos_voice_data_flag flag; //本次数据是否检测到对应事件 [out-reserved]
	//int kws_data_leng_total;//kws所有数据长度[out]
	char *data;      //存放获取数据的地址，此空间是用户管理的 [in]
	int data_length; //作为输入，描述用户想要获取的数据长度，单位字节[in]。作为输出，标识用户实际获取到的数据长度，单位字节       [out]
	int data_end;    //是否是最后一段数据[out] //reserved
	short * data_framestate;   //VAD数据0/1状态缓存
	int data_framestate_length_byte;//VAD数据0/1状态缓存长度，单位byte
}mit_rtos_voice_data;

/*从SDK中获取对应状态的语音数据。
  比如唤醒成功时，获取唤醒切分数据，可以一次性全部获取完毕，也可以分批多次获取；
  识别状态，获取识别数据，由于识别数据是实时获取，因此只能分批次获取，直到获取所有识别数据后结束本次识别过程。
  参数：
  	voice_data： 描述获取数据的相关信息。
  返回值:
    成功返回 0. 数据信息存放在voice_data结构体中。
    失败返回负值。
*/
int mit_rtos_get_voice(mit_rtos_voice_data *voice_data);

/*针对唤醒后播放提示音的场景，通过此接口设置提示音播报的状态 、on时硬件延迟(ms) 、当前录音缓存数据长度(ms)*/
typedef enum {
	kMitRtosPlayOn=2,//正在播报提示音
	kMitRtosPlayOff, //播放已经停止。
}MitRotsPlayState;
int mit_rtos_set_playstate(MitRotsPlayState play_state , int hw_delay, int record_buffer_len);


/*使用内部语音数组，进行唤醒检测验证。唤醒成功返回1，唤醒失败返回0.出错返回负值
 * fe_enable、vad_enable、kws_enable： 算法模块使能与否，默认1即可。
 * rtf_log_mode,调试模式，主要用于显示算法模块的RTF 0x20，或者出错时打印较多内部信息0x01。 
 * alg_type 算法类型。
 * 唤醒成功，返回1. 唤醒失败，返回0.出错返回负值。
*/
int mit_rtos_selfcheck(int fe_enable, int vad_enable, int kws_enable, int rtf_log_mode, MitRtosAlgType alg_type);
int mit_rtos_selfcheck_useconfig(MitRtosConfig * config, int rtf_log_mode);


/*自定义唤醒词传入结构体，通过 WfstInit()中char* cfg传入，进行类型转换*/
/*all the memory pointed by pointer can release after function return*/
typedef struct{
	int count;                   //custom_keyword line_count of phone_sequence_list in 'cfg_file_path'
	float cc_thresh;             //reserved,just set 0.0.
	char * cfg_file_path;        //phone_sequence_list of custom_keyword. include 'keyword statenum state1...stateN 1(mainkeyword)/0(not mainkeyword)\n'
	
	float *g_threshs;            //thresholds         of per phone_sequence. point to a memory of length 'count*sizeof(float)'
	float *g_subseg_confidences; //subseg_confidences of per phone_sequence. point to a memory of length 'count*sizeof(float)'
	float *g_durations_min;      //durations_min      of per phone_sequence. point to a memory of length 'count*sizeof(float)'
	float *g_durations_max;      //durations_max      of per phone_sequence. point to a memory of length 'count*sizeof(float)'
} MitRtosCustomKeywordInfo;

extern int MIT_RTOS_MODE_SWITCH_KWS_RECOVER;

/*MitRtosVadConfig store the config info of vad_module*/
typedef struct{
	int vad_endpoint_delay;//尾点延迟时长，单位ms  设置200
	int vad_silencetimeout;//静音超时时长，单位ms  设置5000
	int vad_voicetimeout;  //语音超时时长，单位ms  设置10000
	float vad_speech_noise_thres_sp;//安静到语音的阈值，起点阈值
	float vad_speech_noise_thres_ep;//语音到安静的阈值，尾点阈值
} MitRtosVadConfig;

/*
 * define the enum of sdk_mode_switch type
 */
typedef enum  {
  kMitRtosModeSwitchTypeNone = 0, // 无切换
  kMitRtosModeSwitchTypeKws     , // change kws module config. value=-512 recover; value=0-100 set
  kMitRtosModeSwitchTypeFE,       // change fe module alg. *just support for fe.agl.6mic*
}MitRtosModeSwitchType;

typedef enum  {
  kMitRtosModeSwitchTypeFEValueNormal = 0, // change FE to normal mode
  kMitRtosModeSwitchTypeFEValueBSS,        // change FE to bss mode
}MitRtosModeSwitchFEValue;

/*
 * define the parameter info of sdk_mode_switch type
 */
typedef struct {
	int switch_type; //sdk_mode_switch type
	int value;       //info of 'switch_type'
}MitRtosModeSwitchInfo;

/*
 * sdk mode switch. 
 * should call after init
 */
int mit_rtos_mode_switch(MitRtosModeSwitchInfo * info);


/*MitRtosSdkKwsThresh 设置阈值的结构体，配合set_str/get_str接口使用*/
typedef struct MitRtosSdkKwsThresh{
	int mode;         // day 0, night 1, /*2 FOTA+day*/
    int thresh_day_;  // 白天阈值
    int thresh_night_;// 夜晚阈值
}MitRtosSdkKwsThresh;

typedef enum  {
  kMitRtosParamIntCmd = 1, //*_int()接口中 是否需要切换commond模式。内部默认开启
  kMitRtosParamFEOutNum,   //*_int()接口中 获取算法FE后数据真实输出通道数
  kMitRtosParamCustomKeyword,    //*_str()接口中 设置自定义唤醒词信息
  kMitRtosParamCustomKeywordDel, //*_str()接口中 删除自定义唤醒词信息
  kMitRtosParamCustomKwsModuleSwitch, //开关KWS模块，热切换，用于识别过程关闭kws检测的场景。
  kMitRtosParamLpmMode, //用户低功耗切换相关参数. 1 enter lpm, 0 exit lpm
  kMitRtosParamSdkSwitchEnable, //算法切换功能开关。默认开启
  kMitRtosParamKwsUpdataExtend, //*_int()接口中,设置kws数据上传扩展功能是否使能
  kMitRtosParamSdkKwsThresh, //算法切换功能开关。默认开启
  kMitRtosParamAsrVadConfig, //*_str()接口中,设置识别专用VAD的配置参数. work with MitRtosVadConfig structure and mit_rtos_get_param_str().
}MitRtosParamInt_t;  


/*设置SDK功能参数，即时生效。需要在init之后调用。
  param_type_int: from enum MitRtosParamInt_t.
  value: set 0-disable/1-enable. 
*/
int mit_rtos_set_param_int(MitRtosParamInt_t param_type_int, int value);
int mit_rtos_get_param_int(MitRtosParamInt_t param_type_int, int *value);
int mit_rtos_get_param_str(MitRtosParamInt_t param_type_int, char* str);
int mit_rtos_set_param_str(MitRtosParamInt_t param_type_int, const char* str);
int mit_rtos_get_param_str(MitRtosParamInt_t param_type_int, char* str);

/*获取SDK版本信息,返回内部字符串. */
const char * mit_rtos_get_version_info();
//int mit_rtos_get_version_info(char version_buffer[128], int length_byte_version_buffer);

/*LPM mode type [start]*/
typedef enum {
	kMitRtosSdkLpmModeNone = 0,  //无相关处理。
	kMitRtosSdkLpmModeFirst = 1, //低功耗过程中无动作，只在第一次切换到正常模式时使用对应流程
}MitRtosSdkLpmMode;

typedef enum {
	kMitRtosSdkLpmModeFENormal = 0,  //FE模块无变化
	kMitRtosSdkLpmModeFEOnlyAGC = 1, //只开启FE.AGC模块，其他不变。
	kMitRtosSdkLpmModeFEBypass	= 2, //FE模块直接bypass，通道数不变
	kMitRtosSdkLpmModeFEOn	= 3,	 //FE模块开启使能，通道数不变。 用于特殊需求，基本不会用到
	//kMitRtosSdkLpmModeFEBypassNovad  = 4, //FE模块直接bypass，通道数不变. kws专用vad关闭。
}MitRtosSdkLpmModeFEType; 
typedef enum {
	kMitRtosSdkLpmModeVadofKWSNormal = 0, //VAD模块无变化
	kMitRtosSdkLpmModeVadofKWSOff	 = 1, //VAD模块关闭
	kMitRtosSdkLpmModeVadofKWSFEOn	 = 2, //VAD模块开启使能，通道数不变。 用于特殊需求，基本不会用到
}MitRtosSdkLpmModeVadofKWSType; 
	
typedef struct {
	int value;       //1 enter lpm ; 0 exit lpm.
	int mode;		 //低功耗模式。默认使用first模式。
	//int first_type;			 //first模式的type设置， 0 和正常模式一样， 1 只有agc模块， 2 bypass。
	int first_type_fe;		   //FE模块配置。0 onlyagc， 1 bypass
	int first_type_vad_of_kws; //kws专用vad配置。 1 开启vad、0 关闭vad
	int first_type_ch_num;	   //通道数。 reserved

	int data_len_ms;		   //数据长度
}MitRtosLpmEngineConfig;
/*LPM mode type [done]*/


#ifdef __cplusplus 
}
#endif
#endif
