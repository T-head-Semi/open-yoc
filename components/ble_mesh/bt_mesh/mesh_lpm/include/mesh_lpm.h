/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __MESH_LPM_H__
#define __MESH_LPM_H__

#define INTERRUPT_DELAY_TIME (50) //For debounce

#define DEFAULT_BOOTUP_DELAY_SLEEP_TIME (10000) //Unit:ms
#define DEF_MAX_IO_WAKEUP_PROCESS_TIME  (2000) //1s

#define PWR_STANDBY_BOOT_FLAG (0x1688)
#define PWR_BOOT_REASON (0x4000f034)




typedef enum _mesh_lpm_status_e {
    STATUS_WAKEUP,
    STATUS_SLEEP
} mesh_lpm_status_e;

typedef enum _mesh_lpm_wakeup_reason_e {
    WAKEUP_BY_IO,
    WAKEUP_BY_TIMER,
    WAKEUP_IS_WAKEUP
} mesh_lpm_wakeup_reason_e;

typedef enum _mesh_lpm_wakeup_ {
    WAKEUP_BY_IO_MODE       = 0x01,
	WAKEUP_BY_TIMER_MODE    = 0x02,
	WAKEUP_BY_IO_TIMER_MODE = 0x03,
} mesh_lpm_wakeup_mode_e;


typedef enum {
    MESH_LPM_MODE_TX_ONLY,
    MESH_LPM_MODE_TX_RX,
    MESH_LPM_MODE_RX_TX,
} mesh_lpm_mode_en;


typedef struct _mesh_lpm_wakeup_io_config_s {
    uint8_t port;
    uint8_t io_pol;
} mesh_lpm_wakeup_io_config_t;

typedef struct _mesh_lpm_wakeup_timer_config_s {
    uint32_t sleep_ms;           //sleep time
    uint32_t wakeup_ms;          //wakeup time
} mesh_lpm_wakeup_timer_config_t;


typedef struct _mesh_lpm_wakeup_io_s {
    uint8_t io_list_size;
    mesh_lpm_wakeup_io_config_t* io_config;
} mesh_lpm_wakeup_io_t;

typedef struct _mesh_lpm_io_status_s {
    uint8_t port;
    uint8_t trigger_flag;
    uint8_t status;
} _mesh_lpm_io_status_t;

typedef struct _mesh_lpm_io_status_list_s {
    _mesh_lpm_io_status_t *io_status;
    uint8_t size;
} _mesh_lpm_io_status_list_t;

typedef void (*mesh_lpm_cb_t)(mesh_lpm_wakeup_reason_e reason, mesh_lpm_status_e status,void* arg);

typedef struct _mesh_lpm_conf_s {
    uint8_t lpm_mode;
    uint8_t lpm_wakeup_mode;       //0:then not wakeup by GPIO
    mesh_lpm_wakeup_io_t  lpm_wakeup_io_config;   //wakeup IO list config
    mesh_lpm_wakeup_timer_config_t lpm_wakeup_timer_config;  //wakeup timer cponfig
    uint8_t is_auto_enable;      //1:auto enter sleep mode when bootup
    uint32_t delay_sleep_time;   //if auto enter sleep,delay some time then enter sleep mode
    mesh_lpm_cb_t mesh_lpm_cb; //User callback
} mesh_lpm_conf_t;


#define MESH_WAKEUP_PIN(_port, _pol)  \
{                      \
	.port   = (_port), \
	.io_pol = (_pol),   \
}



int mesh_lpm_init(mesh_lpm_conf_t *lpm_conf);
int mesh_lpm_start(void);
int mesh_lpm_enable(bool force);
int mesh_lpm_disable(void);
int mesh_lpm_deep_sleep(void);

#endif
