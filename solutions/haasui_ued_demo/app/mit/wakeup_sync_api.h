#ifndef __AILABS__WAKEUP_API_WAKEUP_SYNC_API_H__
#define __AILABS__WAKEUP_API_WAKEUP_SYNC_API_H__


#if (!(defined XENGINE_CALL) || !(defined XENGINE_IMPORT_OR_EXPORT))
#    if defined __WIN32__ || defined _WIN32 || defined _WIN64
#       define XENGINE_CALL __stdcall
#       ifdef  XENGINE_IMPLEMENTION
#           define XENGINE_IMPORT_OR_EXPORT __declspec(dllexport)
#       else
#           define XENGINE_IMPORT_OR_EXPORT __declspec(dllimport)
#       endif
#    elif defined __ANDROID__
#       define XENGINE_CALL
#       define XENGINE_IMPORT_OR_EXPORT __attribute ((visibility("default")))
#    else
#       define XENGINE_CALL
#       define XENGINE_IMPORT_OR_EXPORT __attribute ((visibility("default")))
#    endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 天猫精灵API应用的产品类别
 */
enum TmallAPIDev
{
    kTmallAPIDevDibao=0, //盖亚项目，地宝
    kTmallAPIDevQinbao,  //盖亚项目，沁宝
};

/**
 * 传递的设备状态
 */
enum WkuDevStatus
{
    kWkuDevStatusDefault=0,//初始，不知道什么状态，算法内部不做处理
	kWkuDevStatusWorking,  //工作状态 工作态，清洁中
	kWkuDevStatusGoback,   //工作状态 工作态，回充中
	kWkuDevStatusCleanRag, //工作状态 工作态，清洗抹布中
	kWkuDevStatusCharge,   //工作状态 非工作态，充电中
	kWkuDevStatusStandby,  //工作状态 非工作态，待机
	kWkuDevStatusSleep,    //工作状态 非工作态，休眠
	kWkuDevStatusPause,    //工作状态 非工作态，暂停
};
/**
 * 传递的设备状态相关，工作模式
 */
enum WkuDevStatusWorkMode
{
    kWkuDevStatusWorkModeDefault=0,//初始，不知道什么状态，算法内部不做处理
    kWkuDevStatusWorkModeClean,    //工作模式_扫地模式
    kWkuDevStatusWorkModeCleanMop ,//工作模式_扫拖模式
};
/**
 * 传递的设备状态相关，吸力大小
 */
enum WkuDevStatusSuction
{
    kWkuDevStatusSuctionDefault=0,//初始，不知道什么状态，算法内部不做处理
    kWkuDevStatusSuctionSmall,    //吸力大小_小，对应风机工作 抵挡 
    kWkuDevStatusSuctionMid,      //吸力大小_中，对应风机工作 中档
    kWkuDevStatusSuctionBig,      //吸力大小_大，对应风机工作 高档      /*20210113 只支持此状态下切换*/
    kWkuDevStatusSuctionUltrabig, //吸力大小_超大，对应风机工作 超高档 /*20210113 只支持此状态下切换*/
};
#if 0
enum WkuDevStatus
{
    kWkuDevStatusNone=0, 
    kWkuDevStatusQuite,    //静置（充电座中）/*20210111 只支持此状态下切换*/
//    kWkuDevStatusCruise ,  //巡航中，没有这个场景，可能就是开始工作中。
    kWkuDevStatusWorkClean,//工作模式_扫地
    kWkuDevStatusWorkMop , //工作模式_拖地
    kWkuDevStatusBlock ,   //卡住，对应暂停。
    kWkuDevStatusFanWorkTallWS, //风机工作_高档 /*20210113 只支持此状态下切换*/
    kWkuDevStatusFanWorkMidWS,  //风机工作_中档
    kWkuDevStatusFanWorkLowWS,  //风机工作_低档
//    kWkuDevStatusMotorSpeed ,   //电机转速，对应吸力变化。边刷会有快慢、滚刷转速恒定、吸力会调节。
//    kWkuDevStatusTowards ,      //扫地机的朝向,XYZ 加朝向，不提供了。
    kWkuDevStatusExceptionList ,//异常情况列表
    kWkuDevStatusBatteryInfo   ,//电量信息
    kWkuDevStatusPoweroffTime  ,//断电时间
    kWkuDevStatusOther,

	//风机工作_超高档， 超大吸力
	//卡住
	//任何状态都要支持播放音乐？？ 
	//静止，
	//开始，噪声依赖吸力；清洗抹布，有充水、吸水过程，噪声可能会比最大吸力噪声还大。
	//工作状态中是否播放音乐，科沃斯无特殊需求。如果影响唤醒效果，双方可以再讨论、确认。
	//唤醒时，扫地机工作暂停。
};
#endif

typedef struct wku_devstatus_info{
	int status;    //设备状态相关。工作状态or非工作状态
	int suction;   //设备状态相关。吸力大小
	int work_mode; //设备状态相关。工作模式
}wku_devstatus_info_t; 

typedef struct wku_feed_info{
	const char * addr;  //状态信息首地址
	int length;         //addr中数据长度，byte
	float volume;       //original volume data
    int disable_nnet_;  //是否需要关闭kws流程， 1关闭、0不关闭
}wku_feed_info_t; 
/**
 * 系统状态信息
 * volume_level_：当前系统播放音量
 * audio_type_：当前系统播放类型，1表示音乐，2表示提示音，3表示tts
 * full_duplex_mode_：0表示非全双工模式，1表示全双工模式
 * asr_mode_：0表示非ASR对话模式，1表示ASR对话模式
 * disable_nnet_：0表示跑唤醒模型，1表示不跑唤醒模型
 * feed_sgp_out_：0表示喂原始pcm数据，1表示喂信号处理输出的数据
 */
typedef struct {
    float volume_level_;
    int audio_type_;
    int full_duplex_mode_;
    int asr_mode_;
    int disable_nnet_;
    int feed_sgp_out_;
} SystemInfo;

enum XEgnLogLevel {
    LOGGER_DEBUG = 0,
    LOGGER_INFO,
    LOGGER_WARNING,
    LOGGER_ERROR
};

enum XEgnTimeUnit {
    TU_SECOND = 0,
    TU_MILLISECOND,
    TU_MICROSECOND,
    TU_NANOSECOND
};

/**
 * 唤醒回调函数返回消息类型
 */
enum {
    XENGINE_MESSAGE_TYPE_JSON = 1,
    XENGINE_MESSAGE_TYPE_BIN
};

/**
 * 信号处理回调函数返回消息类型
 * AudioNormal： 正常音频
 * AudioRollbackStart： 唤醒的rollback开始，这里仅仅返回数据大小，数据不可用
 * AudioRollback： 唤醒的rollback数据
 * AudioRollbackEnd： rollback数据结束
 */
enum AudioDataStatus
{
    AudioNormal=0,
    AudioRollbackStart,
    AudioRollback,
    AudioRollbackEnd
};

/**
 * log回调函数
 */
typedef int (*LogHandler)(void *usr_ptr, const char *buf, int bytes, int log_level);

/**
 * 时间回调函数
 */
typedef double (*TimeHandler)(void* time_user, int time_unit);

/**
 * 唤醒回调函数
 * usr_ptr：回调函数数据
 * json：唤醒json串
 * bytes： 唤醒数据大小
 * type： 唤醒数据类型
 */
typedef int (*SyncWkuResultHandler)(void *usr_ptr, const char *json, int bytes, int type);

/**
 * 信号处理回调函数
 * usr_ptr：回调函数数据
 * data：信号处理后的数据
 * size： 信号处理后的数据大小
 * type： 信号处理后的数据类型（AudioDataStatus）
 */
typedef int (*SyncSgpResultHandler)(void *usr_ptr, const char *data, int size, int type);

/**
 * 低阈值唤醒回调函数
 * usr_ptr：回调函数数据
 * json：唤醒json串
 * bytes： 唤醒数据大小
 * type： 唤醒数据类型
 */
typedef int (*SyncLowWkuResultHandler)(void *usr_ptr, const char *json, int bytes, int type);

typedef void*  (*wku_new_func) (const char *cfg_fn, const char* env);
typedef int (*wku_delete_func)(void *engine);
typedef int (*wku_start_func)(void *engine, const char* env, int bytes, void *wku_usr, SyncWkuResultHandler wku_func, void *sgp_usr, SyncSgpResultHandler sgp_func);
typedef int (*wku_feed_func)(void *engine, const char *data, int size, SystemInfo *sysinfo);
typedef int (*wku_stop_func)(void *engine);

typedef const char* (*wku_version_func)();

typedef void (*wku_register_log_func)(LogHandler out_func, void *usr_data, int level);
typedef void (*wku_regist_lowwku_func)(void *engine, void *low_wku_usr, SyncLowWkuResultHandler low_wku_func);

/**
 * log输出的回调函数，默认输出到终端
 * out_func：外部传入的log输出函数
 * usr_data：回调函数的参数
 * level: 输出大于level的log
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT void XENGINE_CALL wku_register_log(LogHandler out_func, void *usr_data, int level);

/**
 * 时间回调函数，默认基于gettimeofday实现
 * time_func：外部传入的时间回调函数
 * time_user：时间回调函数的参数
 * time_unit: 时间单位
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT void XENGINE_CALL wku_register_time(TimeHandler time_func, void *time_user, int time_unit);

/**
 * 低阈值唤醒后收集数据的回调函数
 * wku_func：外部处理低阈值唤醒的回调函数
 * wku_usr：回调函数的参数
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_regist_lowwku(void *engine, void *low_wku_usr, SyncLowWkuResultHandler low_wku_func);

/**
 * 创建唤醒引擎
 * cfg_fn：唤醒资源路径
 * env：唤醒词相关参数，默认传NULL
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT void* XENGINE_CALL wku_new(const char *cfg_fn, const char* env);

/**
 * 更新唤醒引擎配置
 * engine：唤醒句柄
 * config：唤醒配置，等于NULL时表示回滚到FOTA配置
 * bytes：唤醒配置大小，小于等于0时表示回滚到FOTA配置
 * name：唤醒配置名字
 * version：唤醒配置版本
 * 返回值说明：-1表示程序逻辑错误，0x01表示FOTA配置错误，0x02表示配置不是合法JSON格式，0x04表示配置JSON域不完整，0x08表示拼音格式非法
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_update_config(void *engine, const char *config, int bytes, const char *name, const char *version);

/**
 * 销毁唤醒引擎
 * engine：唤醒句柄
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_delete(void *engine);

/**
 * 开始唤醒引擎
 * engine：唤醒句柄
 * env：唤醒词相关参数，默认传NULL
 * bytes： 参数大小，默认传0
 * wku_usr： 唤醒回调函数数据
 * wku_func： 唤醒回调函数
 * sgp_usr： 信号处理回调函数数据
 * sgp_func： 信号处理回调函数
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_start(void *engine, const char* env, int bytes, void *wku_usr, SyncWkuResultHandler wku_func, void *sgp_usr, SyncSgpResultHandler sgp_func);

/**
 * feed 数据
 * engine：唤醒句柄
 * data：四路麦克风数据
 * bytes：四路麦克风数据大小
 * sysinfo：当前系统状态信息
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_feed(void *engine, const char *data, int size, SystemInfo *sysinfo);

/**
 * 结束唤醒引擎
 * engine：唤醒句柄
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_stop(void *engine);

/**
 * 获取版本号
 * engine：唤醒句柄
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_version_all(void *engine);

/**
 * 处理指令
 * engine: 唤醒句柄
 * function：需要执行的函数
 * param：需要执行的函数的参数
 * result：处理结果缓存
 * size：结果缓存大小
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_handle_command(void *engine, const char *function, const char *param, char *result, int size);

XENGINE_IMPORT_OR_EXPORT const char* XENGINE_CALL wku_version();

#define MAX_CHN_LEN 64
#define MAX_PINYIN_LEN 64
#define MAX_WKUWORD_LEN 128
#define MAX_RELATE_WORDS_CNT 2

typedef struct wku_word_st
{
    char    word_chn_[MAX_CHN_LEN];
    char    pinyin_[MAX_PINYIN_LEN];
    char    wku_word_[MAX_WKUWORD_LEN];
    float   thresh_day_;
    float   thresh_night_;
    float   thresh_low_;
    int     is_asr_recheck_;
    int     is_pre_;
    int     is_invalid_;
    int     relate_words_cnt_;
    char    relate_words_chn_[MAX_RELATE_WORDS_CNT][MAX_CHN_LEN];
}WkuWords;

/**
 * 获取唤醒词阈值
 * engine：唤醒句柄
 * word_chn_：根据中文获取唤醒词信息，填充该字段
 * pinyin_：若word_chn_非空，使用若word_chn_非空获取唤醒词信息，该字段为返回值；若word_chn_为空，根据拼音获取唤醒词信息，填充pinyin_,word_chn_需要置空
 * wku_word_：返回词信息
 * thresh_day_：白天模式阈值
 * thresh_night_：夜间模式阈值
 * thresh_low_: 双阈值中的低阈值
 * is_asr_recheck_: 是否做asr校验
 * is_pre_：是否时预定义的唤醒词，有预定义的阈值
 * is_valid_：唤醒词质量
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_get_threshhold(void *engine, WkuWords *wku_word);

/**
 * 校验asr的结果
 * engine：唤醒句柄
 * wku_word：快捷唤醒词
 * asr_rst：asr识别结果，完整json串
 * 同步接口
 */
XENGINE_IMPORT_OR_EXPORT int XENGINE_CALL wku_recheck_asr(void *engine, char *wku_word, char *asr_rst, int *is_wku);


#ifdef __cplusplus
}
#endif
#endif

