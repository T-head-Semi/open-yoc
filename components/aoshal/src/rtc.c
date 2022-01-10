/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <string.h>
#include "soc.h"
#include "aos/hal/rtc.h"
#include "drv/rtc.h"

static csi_rtc_t rtc_handlers;

#define RTC_BASE_DEC_YEAR_2000                  (2000)
#define RTC_BASE_BCD_YEAR_2000                  (0x2000)

#define ALG_RTC_BCD2DEC(code)                   (((code) & 0xF000) / 4096 * 1000 + ((code) & 0xF00) / 256 * 100 + ((code) & 0xF0) / 16 * 10 + ((code) & 0xF))
#define ALG_RTC_DEC2BCD(code)                   ((code) / 1000 * 4096 + ((code) - (code) / 1000 * 1000) / 100 * 256 + ((code) - (code) / 100 * 100) / 10 * 16 + ((code) - (code) / 10 * 10))
#define ALG_RTC_CAL_WEEK(year, mon, day)        ((day + 2*mon + 3*(mon+1)/5 + year + year/4 - year/100 + year/400))

/**
 * This function will initialize the on board CPU real time clock
 *
 *
 * @param[in]  rtc  rtc device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_rtc_init(rtc_dev_t *rtc)
{
    int ret = 0;

    if (rtc == NULL) {
        return -1;
    }

    ret = csi_rtc_init(&rtc_handlers, rtc->port);

    if (ret < 0) {
        return -1;
    }

    return 0;
}

/**
 * This function will return the value of time read from the on board CPU real time clock.
 *
 * @param[in]   rtc   rtc device
 * @param[out]  time  pointer to a time structure
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_rtc_get_time(rtc_dev_t *rtc, rtc_time_t *time)
{
    int ret = 0;
    csi_rtc_time_t tim;

    if (rtc == NULL || time == NULL) {
        return -1;
    }

    memset(&tim, 0, sizeof(tim));
    ret = csi_rtc_get_time(&rtc_handlers, &tim);

    if (ret < 0) {
        return -1;
    }

    switch (rtc->config.format) {
        case HAL_RTC_FORMAT_BCD:
            time->sec       = ALG_RTC_DEC2BCD(tim.tm_sec);
            time->min       = ALG_RTC_DEC2BCD(tim.tm_min);
            time->hr        = ALG_RTC_DEC2BCD(tim.tm_hour);
            time->weekday   = ALG_RTC_DEC2BCD(tim.tm_wday);
            time->date      = ALG_RTC_DEC2BCD(tim.tm_mday);
            time->month     = ALG_RTC_DEC2BCD(tim.tm_mon) + 1;
            time->year      = ALG_RTC_DEC2BCD(tim.tm_year + 1900 - RTC_BASE_DEC_YEAR_2000);
            break;

        case HAL_RTC_FORMAT_DEC:
            time->sec       = tim.tm_sec;
            time->min       = tim.tm_min;
            time->hr        = tim.tm_hour;
            time->weekday   = tim.tm_wday;
            time->date      = tim.tm_mday;
            time->month     = tim.tm_mon + 1;
            time->year      = tim.tm_year + 1900 - RTC_BASE_DEC_YEAR_2000;
            break;

        default:
            return -1;
    }

    return 0;
}
/**
 * This function will set MCU RTC time to a new value.
 *
 * @param[in]   rtc   rtc device
 * @param[out]  time  pointer to a time structure
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_rtc_set_time(rtc_dev_t *rtc, const rtc_time_t *time)
{
    int ret = 0;
    csi_rtc_time_t tim;

    if (rtc == NULL || time == NULL) {
        return -1;
    }

    memset(&tim, 0, sizeof(tim));

    switch (rtc->config.format) {
        case HAL_RTC_FORMAT_BCD:
            tim.tm_sec      = ALG_RTC_BCD2DEC(time->sec);
            tim.tm_min      = ALG_RTC_BCD2DEC(time->min);
            tim.tm_hour     = ALG_RTC_BCD2DEC(time->hr);
            tim.tm_wday     = ALG_RTC_BCD2DEC(time->weekday);
            tim.tm_mday     = ALG_RTC_BCD2DEC(time->date);
            tim.tm_mon      = ALG_RTC_BCD2DEC(time->month) - 1;
            tim.tm_year     = ALG_RTC_BCD2DEC(time->year + RTC_BASE_BCD_YEAR_2000) - 1900;
            break;

        case HAL_RTC_FORMAT_DEC:
            tim.tm_sec      = time->sec;
            tim.tm_min      = time->min;
            tim.tm_hour     = time->hr;
            tim.tm_wday     = time->weekday;
            tim.tm_mday     = time->date;
            tim.tm_mon      = time->month - 1;
            tim.tm_year     = time->year + RTC_BASE_DEC_YEAR_2000 - 1900;
            break;

        default:
            return -1;
    }

    ret = csi_rtc_set_time(&rtc_handlers, (const csi_rtc_time_t *)&tim);

    if (ret < 0) {
        return -1;
    }

    return 0;
}
/**
 * De-initialises an RTC interface, Turns off an RTC hardware interface
 *
 * @param[in]  RTC  the interface which should be de-initialised
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_rtc_finalize(rtc_dev_t *rtc)
{
    if (rtc == NULL) {
        return -1;
    }

    csi_rtc_uninit(&rtc_handlers);

    return 0;
}
