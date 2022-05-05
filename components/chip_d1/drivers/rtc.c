#include <drv/rtc.h>
#include <drv/irq.h>
#include <sunxi_hal_rtc.h>

static csi_rtc_t *rtc_dev;

static void csi_2_d1_rtctime(const csi_rtc_time_t *src, struct rtc_time *dst)
{
    dst->tm_sec = src->tm_sec;
    dst->tm_min = src->tm_min;
    dst->tm_hour = src->tm_hour;
    dst->tm_mday = src->tm_mday;
    dst->tm_mon = src->tm_mon;
    dst->tm_year = src->tm_year;
    dst->tm_wday = src->tm_wday;
    dst->tm_yday = src->tm_yday;
    dst->tm_isdst = 0;
}

static void d1_2_csi_rtctime(struct rtc_time *src, csi_rtc_time_t *dst)
{
    dst->tm_sec = src->tm_sec;
    dst->tm_min = src->tm_min;
    dst->tm_hour = src->tm_hour;
    dst->tm_mday = src->tm_mday;
    dst->tm_mon = src->tm_mon;
    dst->tm_year = src->tm_year;
    dst->tm_wday = src->tm_wday;
    dst->tm_yday = src->tm_yday;
}

static int irq_event(void)
{
    printf("alarm interrupt\n");
    if (rtc_dev->callback) {
        rtc_dev->callback(rtc_dev, rtc_dev->arg);
    }
    return 0;
}

csi_error_t csi_rtc_init(csi_rtc_t *rtc, uint32_t idx)
{
    // rtc->priv = ;
    aw_hal_rtc_init();
    rtc_dev = rtc;

    return 0;
}

void csi_rtc_uninit(csi_rtc_t *rtc)
{
    hal_rtc_deinit();
    rtc_dev = NULL;
}

csi_error_t csi_rtc_set_time(csi_rtc_t *rtc, const csi_rtc_time_t *rtctime)
{
    struct rtc_time rtc_tm;
    csi_2_d1_rtctime(rtctime, &rtc_tm);
    hal_rtc_settime(&rtc_tm);

    return 0;
}

csi_error_t csi_rtc_set_time_no_wait(csi_rtc_t *rtc, const csi_rtc_time_t *rtctime)
{
    struct rtc_time rtc_tm;
    csi_2_d1_rtctime(rtctime, &rtc_tm);
    hal_rtc_settime(&rtc_tm);

    return 0;
}

csi_error_t csi_rtc_get_time(csi_rtc_t *rtc, csi_rtc_time_t *rtctime)
{
    struct rtc_time rtc_tm;
 
    hal_rtc_gettime(&rtc_tm);

    d1_2_csi_rtctime(&rtc_tm, rtctime);

    return 0;
}

uint32_t csi_rtc_get_alarm_remaining_time(csi_rtc_t *rtc)
{
    struct rtc_wkalrm wkalrm;

    hal_rtc_getalarm(&wkalrm);

    return wkalrm.time.tm_sec;
}

csi_error_t csi_rtc_set_alarm(csi_rtc_t *rtc, const csi_rtc_time_t *rtctime, void *callback, void *arg)
{
    struct rtc_wkalrm wkalrm;
    struct rtc_time rtc_tm;

    wkalrm.enabled = 1;

    rtc->callback = callback;
    rtc->arg = arg;

    csi_2_d1_rtctime(rtctime, &rtc_tm);
    wkalrm.time = rtc_tm;
    hal_rtc_register_callback(irq_event);
    hal_rtc_setalarm(&wkalrm);

    return 0;
}

csi_error_t csi_rtc_cancel_alarm(csi_rtc_t *rtc)
{

    return 0;
}

bool csi_rtc_is_running(csi_rtc_t *rtc)
{
    return 0;
}

