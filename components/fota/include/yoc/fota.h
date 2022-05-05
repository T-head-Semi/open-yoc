/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __FOTA_H__
#define __FOTA_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <time.h>
#include <aos/list.h>
#include <aos/kernel.h>
#include <yoc/netio.h>

#define FW_URL_KEY  "FW_URL_KEY"
#define KV_FOTA_OFFSET "fota_offset"
#define KV_FOTA_CLOUD_PLATFORM "fota_cldp"
#define KV_FOTA_FROM_URL "fota_fromurl"
#define KV_FOTA_TO_URL "fota_tourl"
#define KV_FOTA_READ_TIMEOUTMS "fota_rtmout"
#define KV_FOTA_WRITE_TIMEOUTMS "fota_wtmout"
#define KV_FOTA_RETRY_COUNT "fota_retry"
#define KV_FOTA_SLEEP_TIMEMS "fota_slptm"
#define KV_FOTA_AUTO_CHECK "fota_autock"
#define KV_FOTA_FINISH "fota_finish"

#ifndef CONFIG_FOTA_TASK_STACK_SIZE
#define CONFIG_FOTA_TASK_STACK_SIZE (8 * 1024)
#endif

// use httpclient
#ifndef CONFIG_FOTA_USE_HTTPC
#define CONFIG_FOTA_USE_HTTPC 0
#endif

#ifndef CONFIG_FOTA_DATA_IN_RAM
#define CONFIG_FOTA_DATA_IN_RAM 0
#endif

typedef struct fota fota_t;

typedef enum {
    FOTA_EVENT_START = 0,       /*!< Start the fota version check and download steps */
    FOTA_EVENT_VERSION,         /*!< Check version from server ok */
    FOTA_EVENT_PROGRESS,        /*!< Downloading the fota data */
    FOTA_EVENT_FAIL,            /*!< This event occurs when there are any errors during execution */
    FOTA_EVENT_VERIFY,          /*!< verify fota data */
    FOTA_EVENT_FINISH,          /*!< fota download flow finish */
    FOTA_EVENT_QUIT,            /*!< Fota task quit */
    FOTA_EVENT_RESTART          /*!< real want to restart */
} fota_event_e;

typedef enum {
    FOTA_ERROR_NULL = 0,
    FOTA_ERROR_VERSION_CHECK,
    FOTA_ERROR_PREPARE,
    FOTA_ERROR_NET_SEEK,
    FOTA_ERROR_NET_READ,
    FOTA_ERROR_WRITE,
    FOTA_ERROR_MALLOC,
    FOTA_ERROR_VERIFY
} fota_error_code_e;

typedef enum fota_status {
    FOTA_INIT = 1,          /*!< create fota task, wait for version check */
    FOTA_DOWNLOAD = 2,      /*!< start to download fota data */
    FOTA_ABORT = 3,         /*!< read or write exception */
    FOTA_FINISH = 4,        /*!< download finish */
} fota_status_e;

typedef struct {
    char *cur_version;     /*!< the local image version, read from kv*/
    char *local_changelog; /*!< the local image changelog, read from kv*/
    char *new_version;     /*!< the incoming image version, read from cloud server*/
    char *changelog;       /*!< the incoming image changelog, read from cloud server*/
    char *fota_url;        /*!< the incoming image url, read from cloud server*/
    int timestamp;         /*!< the incoming image timestamp, read from cloud server*/
} fota_info_t;

typedef struct fota_cls {
    const char *name;
    int (*init)(fota_info_t *info);
    int (*version_check)(fota_info_t *info);
    int (*finish)(fota_info_t *info);
    int (*fail)(fota_info_t *info);
    int (*restart)(void);
} fota_cls_t;

typedef struct {
    int read_timeoutms;         /*!< read timeout, millisecond */
    int write_timeoutms;        /*!< write timeout, millisecond */
    int retry_count;            /*!< when download abort, it will retry to download again in retry_count times */
    int sleep_time;             /*!< the sleep time for auto-check task */
    int auto_check_en;          /*!< whether check version automatic */
} fota_config_t;

typedef int (*fota_event_cb_t)(void *fota, fota_event_e event);   ///< fota Event call back.

struct fota {
    const fota_cls_t *cls;          /*!< the fota server ops */

    netio_t *from;                  /*!< the read netio handle */
    netio_t *to;                    /*!< the write netio handle */
    fota_status_e status;           /*!< the fota status, see enum `fota_status_e` */
    char *from_path;                /*!< where the fota data read from, url format */
    char *to_path;                  /*!< where the fota data write to, url format*/
    uint8_t *buffer;                /*!< buffer for reading data from net */
    int offset;                     /*!< downloaded data bytes */
    int total_size;                 /*!< total length of fota data */
    int quit;                       /*!< fota task quit flag */
    aos_task_t task;                /*!< fota task handle */
    aos_sem_t sem;                  /*!< semaphore for waiting fota task quit */
    aos_sem_t sem_download;         /*!< semaphore for starting download */
    aos_sem_t do_check_event;       /*!< the semaphore for checking version loop or force */
    fota_event_cb_t event_cb;       /*!< the event callback */
    fota_error_code_e error_code;   /*!< fota error code, get it when event occurs */
    fota_config_t config;           /*!< fota config */
    fota_info_t info;               /*!< fota information */
    aos_timer_t restart_timer;      /*!< the timer to norify to restart */
    void *private;                  /*!< user data context */
};

/**
 * @brief  创建FOTA服务
 * @param  [in] fota: fota 句柄
 * @return 0 on success, -1 on failed
 */
int fota_start(fota_t *fota);

/**
 * @brief  停止FOTA功能，退出FOTA服务
 * @param  [in] fota: fota 句柄
 * @return 0 on success, -1 on failed
 */
int fota_stop(fota_t *fota);

/**
 * @brief  开始下载
 * @param  [in] fota: fota 句柄
 * @return 0 on success, -1 on failed
 */
int fota_download(fota_t *fota);

/**
 * @brief  触发重启并开始升级
 * @param  [in] fota: fota 句柄
 * @param  [in] delay_ms: 延时多少毫秒才重启
 * @return 0 on success, -1 on failed
 */
int fota_restart(fota_t *fota, int delay_ms);

/**
 * @brief  配置FOTA参数
 * @param  [in] fota: fota 句柄
 * @param  [in] config: 配置数据指针，具体见 `fota_config_t`
 */
void fota_config(fota_t *fota, fota_config_t *config);

/**
 * @brief  获取FOTA参数
 * @param  [in] fota: fota 句柄
 * @return 配置数据指针，具体见 `fota_config_t`
 */
fota_config_t *fota_get_config(fota_t *fota);

/**
 * @brief  设置是否自动不断检测服务器固件版本并升级的功能
 * @param  [in] fota: fota 句柄
 * @param  [in] enable: 0表示不自动检测，1表示自动检测
 */
void fota_set_auto_check(fota_t *fota, int enable);

/**
 * @brief  获取是否自动检测判断
 * @param  [in] fota: fota 句柄
 * @return 0表示不自动检测，1表示自动检测
 */
int fota_get_auto_check(fota_t *fota);

/**
 * @brief  获取升级状态
 * @param  [in] fota: fota 句柄
 * @return fota_status_e
 */
fota_status_e fota_get_status(fota_t *fota);

/**
 * @brief  fota初始化
 * @param  [in] fota_name: FOTA平台名字，比如"cop",
 * @param  [in] dst: 差分包存储url
 * @param  [in] event_cb: 用户事件回调
 * @return fota句柄或者NULL
 */
fota_t *fota_open(const char *fota_name, const char *dst, fota_event_cb_t event_cb);

/**
 * @brief  关闭FOTA功能，释放所有资源
 * @param  [in] fota: fota 句柄
 * @return 0 on success, -1 on failed
 */
int fota_close(fota_t *fota);

/**
 * @brief  强制检测版本
 * @param  [in] fota: fota 句柄
 */
void fota_do_check(fota_t *fota);

/**
 * @brief  注册为cop平台，即从cop平台下载固件
 * @return 0 on success, -1 on failed
 */
int fota_register_cop(void);

/**
 * @brief  注册平台接口
 * @param  [in] cls: 不同平台实现的接口集合，具体实现接口见`fota_cls_t`
 * @return 0 on success, -1 on failed
 */
int fota_register(const fota_cls_t *cls);

/**
 * @brief  对已经下载好的FOTA数据进行校验,(用户可自定义)
 * @return 0 on success, -1 on failed
 */
int fota_data_verify(void);

#if CONFIG_FOTA_DATA_IN_RAM > 0
/**
 * @brief  获取存储fota数据的ram地址,(用户可自定义)
 * @return address
 */
unsigned long fota_data_address_get(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
