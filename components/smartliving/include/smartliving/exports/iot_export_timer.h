/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef __IOT_EXPORT_TIMER_H__
#define __IOT_EXPORT_TIMER_H__

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif
#ifdef AIOT_DEVICE_TIMER_ENABLE
#include "iotx_log.h"

    #define DEVICETIMER "DeviceTimer"
    #define DeviceTimerSize 13

    #define DS "dev_timer"
    #define DS_ERR(...)           log_err(TS, __VA_ARGS__)
    #define DS_WRN(...)           log_warning(TS, __VA_ARGS__)
    #define DS_INFO(...)          log_info(TS, __VA_ARGS__)
    #define DS_DEBUG(...)         log_debug(TS, __VA_ARGS__)
    typedef void (*devicetimer_callback)(const char *report_data, const char *property_name, const char *data);
    int aiot_device_timer_init(const char **devicetimer_list, uint8_t num_devicetimer_list, devicetimer_callback timer_service_cb);
    int deviceTimerParse(const char *input, uint8_t src, int save);
    int aiot_device_timer_inited(void);
    int aiot_device_timer_clear(void);
#else
#define NUM_OF_CONTROL_TARGETS 30
#define NUM_OF_TARGETS_FLOAT 10
#define NUM_OF_TARGETS_STRING 3
#define NUM_OF_TSL_TYPES 3  /*   1:int/enum/bool; 2:float/double; 3:text/date   */

#define STRING_MAX_LEN  80
#define DAYS_OF_WEEK    7
// #define ENABLE_LED_HSV
// #define ENABLE_LED_RGB
// #define ENABLE_LED_HSL

void timer_service_clear(void);

typedef void (*callback_fun)(const char *report_data, const char *property_name, int i_value, 
							 double d_value, const char * s_value, int prop_idx);
typedef void (*callback_ntp_fun)();

int timer_service_init(const char **control_list, uint8_t num_control_list, 
                                    const char **countdownlist_target, uint8_t num_countdownlist_target, 
                                    const char **localtimer_list, uint8_t num_localtimer_list, 
                                    callback_fun timer_service_cb, int *num_of_tsl_type, callback_ntp_fun timer_ntp_cb);
int timer_service_property_set(const char* data);
char *timer_service_property_get(const char *request);

#endif

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif

#endif
