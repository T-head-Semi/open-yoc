/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

/* AOS includes */

#include <aos/aos.h>
#include "k_api.h"
#include <osdep_service.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
/********************* os depended utilities ********************/

#ifndef USE_MUTEX_FOR_SPINLOCK
#define USE_MUTEX_FOR_SPINLOCK 1
#endif

//PRIORITIE_OFFSET  defined to adjust the priority of threads in wlan_lib
unsigned int g_prioritie_offset = 4;

//----- ------------------------------------------------------------------
// Misc Function
//----- ------------------------------------------------------------------

#define DBG_OS_API 0

/*
 * g_wifi_test_en = 0 for wifi log off
 * g_wifi_test_en = 1 for wifi command log
 * g_wifi_test_end = 2 for wifi all log
 */
static int g_wifi_test_en = 0;
void hal_wifi_test_enabled(int en){
    g_wifi_test_en = en;
}

extern int vasprintf(char **strp, const char *fmt, va_list ap);
int rtw_printf(const char *format, ...)
{
    char *buffer;
    if (g_wifi_test_en) {

        va_list args;

        va_start(args, format);
        int n = vasprintf(&buffer, format, args);
        va_end(args);
        if (g_wifi_test_en == 1) {
            if (strncmp(buffer, "RTL871X:", 8) == 0 || strncmp(buffer, "[BTCoex]", 8) == 0 || strcmp(buffer, "\n\r") == 0) {
                free(buffer);
                return 0;
            }
        }

        printf("%s", buffer);
        fflush(stdout);
        free(buffer);
        return n;
    }
	return 0;
}

int rtw_mplog(const char *format, ...)
{
    return 0;
}

void save_and_cli()
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    //taskENTER_CRITICAL();
    aos_kernel_sched_suspend();

}

void restore_flags()
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    //taskEXIT_CRITICAL();
    aos_kernel_sched_resume();
}

void cli()
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    //taskDISABLE_INTERRUPTS();
    aos_kernel_sched_suspend();
}

/* Not needed on 64bit architectures */
static unsigned int __div64_32(u64 *n, unsigned int base)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    u64 rem = *n;
    u64 b = base;
    u64 res, d = 1;
    unsigned int high = rem >> 32;

    /* Reduce the thing a bit first */
    res = 0;

    if (high >= base) {
        high /= base;
        res = (u64) high << 32;
        rem -= (u64)(high * base) << 32;
    }

    while ((u64)b > 0 && b < rem) {
        b = b + b;
        d = d + d;
    }

    do {
        if (rem >= b) {
            rem -= b;
            res += d;
        }

        b >>= 1;
        d >>= 1;
    } while (d);

    *n = res;
    return rem;
}

/********************* os depended service ********************/

u8 *_aos_malloc(u32 sz)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    return aos_malloc(sz);
}

u8 *_aos_zmalloc(u32 sz)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    u8 *ptr = aos_zalloc(sz);

    if (ptr == NULL) {
        printf("ptr failed\n");
    }

    return ptr;
}

void _aos_mfree(u8 *pbuf, u32 sz)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_free(pbuf);
}

static void _aos_memcpy(void *dst, void *src, u32 sz)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    memcpy(dst, src, sz);
}

static int _aos_memcmp(void *dst, void *src, u32 sz)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    if (!(memcmp(dst, src, sz))) {
        return _SUCCESS;
    }

    return _FAIL;
}

static void _aos_memset(void *pbuf, int c, u32 sz)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    memset(pbuf, c, sz);
}

static void _aos_init_sema(_sema *sema, int init_val)
{
    aos_sem_t *aos_sema = aos_malloc(sizeof(aos_sem_t));
    aos_sem_new(aos_sema, init_val);
    *sema = aos_sema;
#if DBG_OS_API
    printf("[AOS]%s %x , %x\n", __FUNCTION__, *sema, __builtin_return_address(0));
#endif

}

static void _aos_free_sema(_sema *sema)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    if (*sema != NULL) {
        aos_sem_free(*sema);
        aos_free(*sema);
    }

    *sema = NULL;
}

static void _aos_up_sema(_sema *sema)
{
#if DBG_OS_API
    printf("[AOS]%s %d\n", __FUNCTION__, *sema);
#endif
    aos_sem_signal(*sema);
}

static void _aos_up_sema_from_isr(_sema *sema)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_sem_signal(*sema);
}

static u32 _aos_down_sema(_sema *sema, u32 timeout)
{
#if DBG_OS_API
    printf("[AOS]%s %x\n", __FUNCTION__, *sema);
#endif

    if (aos_sem_wait(*sema, timeout) != 0) {
        return _FAIL;
    }

    return _SUCCESS;
}

static void _aos_mutex_init(_mutex *pmutex)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    _aos_init_sema(pmutex, 1);
}

static void _aos_mutex_free(_mutex *pmutex)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    if (*pmutex != NULL) {
        _aos_free_sema(pmutex);
    }

    *pmutex = NULL;
}

static void _aos_mutex_get(_lock *plock)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    int ret = _aos_down_sema(plock, -1);

    if (ret != _SUCCESS) {
        printf("_aos_mutex_get get failed\n");
    }
}

static int _aos_mutex_get_timeout(_lock *plock, u32 timeout_ms)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    int ret = _aos_down_sema(plock, timeout_ms);

    if (ret != _SUCCESS) {
        printf("_aos_mutex_get_timeout get failed\n");
        return _FAIL;
    }

    return _SUCCESS;
}

static void _aos_mutex_put(_lock *plock)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    _aos_up_sema(plock);
}

static void _aos_enter_critical(_lock *plock, _irqL *pirqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_kernel_sched_suspend();
}

static void _aos_exit_critical(_lock *plock, _irqL *pirqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_kernel_sched_resume();
}

//static u32 uxSavedInterruptStatus = 0;
static void _aos_enter_critical_from_isr(_lock *plock, _irqL *pirqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_kernel_sched_suspend();
}

static void _aos_exit_critical_from_isr(_lock *plock, _irqL *pirqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_kernel_sched_resume();
}

static int _aos_enter_critical_mutex(_mutex *pmutex, _irqL *pirqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    int ret = _aos_down_sema(pmutex, 60 * 1000);

    if (ret != _SUCCESS) {
        printf("Mutex3 get failed\n");
        return _FAIL;
    }

    return _SUCCESS;
}

static void _aos_exit_critical_mutex(_mutex *pmutex, _irqL *pirqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    _aos_up_sema(pmutex);
}

/** refer to aos_port.c from bluetooth */
static void _aos_cpu_lock(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if defined(CONFIG_PLATFORM_8195BHP)
    __disable_irq();
    icache_disable();
    dcache_disable();

    gtimer_init(&tmp_timer_obj, 0xff);
    gtimer_reload(&tmp_timer_obj, 400 * 1000);  // 4s
    gtimer_start(&tmp_timer_obj);
#endif

    save_and_cli();

}

static void _aos_cpu_unlock(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if defined(CONFIG_PLATFORM_8195BHP)
    int duration = (int)gtimer_read_us(&tmp_timer_obj) / 1000;
    gtimer_deinit(&tmp_timer_obj);
    // compensate rtos tick
    vTaskIncTick(duration);
    dcache_enable();
    icache_enable();
    icache_invalidate();
    __enable_irq();
#endif

    restore_flags();
}

static void _aos_spinlock_init(_lock *plock)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if USE_MUTEX_FOR_SPINLOCK
    _aos_mutex_init(plock);
#endif

}

static void _aos_spinlock_free(_lock *plock)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if USE_MUTEX_FOR_SPINLOCK
    _aos_mutex_free(plock);
#endif
}

static void _aos_spinlock(_lock *plock)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if USE_MUTEX_FOR_SPINLOCK
    _aos_mutex_get(plock);
#endif
}

static void _aos_spinunlock(_lock *plock)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if USE_MUTEX_FOR_SPINLOCK
    _aos_mutex_put(plock);
#endif
}

static void _aos_spinlock_irqsave(_lock *plock, _irqL *irqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    _aos_enter_critical(plock, irqL);
#if USE_MUTEX_FOR_SPINLOCK
    _aos_mutex_get(plock);
#endif
}

static void _aos_spinunlock_irqsave(_lock *plock, _irqL *irqL)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if USE_MUTEX_FOR_SPINLOCK
    _aos_mutex_put(plock);
#endif
    _aos_exit_critical(plock, irqL);
}

typedef struct xqueue_desc_t {
    aos_queue_t queue_hdl;
    int message_size;
    char *buf;
    char name[10];
} xqueue_desc;

static int _aos_init_xqueue(_xqueue *queue, const char *name, u32 message_size, u32 number_of_messages)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    char *buf = aos_malloc(message_size * number_of_messages);

    if (buf == NULL) {
        return _FAIL;
    }

    xqueue_desc *desc = aos_malloc(sizeof(xqueue_desc));
    desc->message_size = message_size;
    desc->buf = buf;
    strncpy(desc->name, name, sizeof(desc->name) - 1);

    if (aos_queue_new(&desc->queue_hdl, buf, message_size * number_of_messages, message_size) != 0) {
        aos_free(desc);
        aos_free(buf);
        return _FAIL;
    }

    *queue = desc;
    return _SUCCESS;
}

static int _aos_push_to_xqueue(_xqueue *queue, void *message, u32 timeout_ms)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    xqueue_desc *desc = (xqueue_desc *)*queue;

    if (aos_queue_send(&desc->queue_hdl, message, desc->message_size) != 0) {
        return _FAIL;
    }

    return _SUCCESS;
}

static int _aos_pop_from_xqueue(_xqueue *queue, void *message, u32 timeout_ms)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    xqueue_desc *desc = (xqueue_desc *)*queue;

    if (timeout_ms == RTW_WAIT_FOREVER) {
        timeout_ms = 1000 * 1000 * 1000;
    }

    if (aos_queue_recv(&desc->queue_hdl, timeout_ms, message, (size_t*)&desc->message_size) != 0) {
        return _FAIL;
    }

    return _SUCCESS;
}

static int _aos_deinit_xqueue(_xqueue *queue)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    xqueue_desc *desc = (xqueue_desc *)*queue;

    aos_queue_free(&desc->queue_hdl);
    aos_free(desc->buf);
    aos_free(desc);

    return _SUCCESS;
}

static u32 _aos_get_current_time(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    return krhino_sys_tick_get();
}

static u32 _aos_systime_to_ms(u32 systime)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    return krhino_ticks_to_ms(systime);;
}

static u32 _aos_systime_to_sec(u32 systime)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    return krhino_ticks_to_ms(systime) / 1000;
}

static u32 _aos_ms_to_systime(u32 ms)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    return krhino_ms_to_ticks(ms);
}

static u32 _aos_sec_to_systime(u32 sec)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    return krhino_ms_to_ticks(sec * 1000);
}

static void _aos_msleep_os(int ms)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_msleep(ms);
}

static void _aos_usleep_os(int us)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_msleep(us / 1000);
}

static void _aos_mdelay_os(int ms)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_msleep(ms);
}

static void _aos_udelay_os(int us)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_msleep(us / 1000);
}

static void _aos_yield_os(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_task_yield();

}

static void _aos_ATOMIC_SET(ATOMIC_T *v, int i)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    save_and_cli();
    v->counter = i;
    restore_flags();
}

static int _aos_ATOMIC_READ(ATOMIC_T *v)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    return v->counter;
}

static void _aos_ATOMIC_ADD(ATOMIC_T *v, int i)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    save_and_cli();
    v->counter += i;
    restore_flags();
}

static void _aos_ATOMIC_SUB(ATOMIC_T *v, int i)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    save_and_cli();
    v->counter -= i;
    restore_flags();

}

static void _aos_ATOMIC_INC(ATOMIC_T *v)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    _aos_ATOMIC_ADD(v, 1);
}

static void _aos_ATOMIC_DEC(ATOMIC_T *v)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    _aos_ATOMIC_SUB(v, 1);
}

static int _aos_ATOMIC_ADD_RETURN(ATOMIC_T *v, int i)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    int temp;

    save_and_cli();
    temp = v->counter;
    temp += i;
    v->counter = temp;
    restore_flags();

    return temp;
}

static int _aos_ATOMIC_SUB_RETURN(ATOMIC_T *v, int i)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    int temp;

    save_and_cli();
    temp = v->counter;
    temp -= i;
    v->counter = temp;
    restore_flags();

    return temp;
}

static int _aos_ATOMIC_INC_RETURN(ATOMIC_T *v)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    return _aos_ATOMIC_ADD_RETURN(v, 1);
}

static int _aos_ATOMIC_DEC_RETURN(ATOMIC_T *v)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    return _aos_ATOMIC_SUB_RETURN(v, 1);
}

static u64 _aos_modular64(u64 n, u64 base)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    unsigned int __base = (base);
    unsigned int __rem;

    if (((n) >> 32) == 0) {
        __rem = (unsigned int)(n) % __base;
        (n) = (unsigned int)(n) / __base;
    } else {
        __rem = __div64_32(&(n), __base);
    }

    return __rem;
}

/* Refer to ecos bsd tcpip codes */
static int _aos_arc4random(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    u32 res = aos_now_ms();
    static unsigned long seed = 0xDEADB00B;

    seed = ((seed & 0x007F00FF) << 7) ^
           ((seed & 0x0F80FF00) >> 8) ^ // be sure to stir those low bits
           (res << 13) ^ (res >> 9);    // using the clock too!
    return (int)seed;
}

static int _aos_get_random_bytes(void *buf, u32 len)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
#if 1 //becuase of 4-byte align, we use the follow code style.
    unsigned int ranbuf;
    unsigned int *lp;
    int i, count;
    count = len / sizeof(unsigned int);
    lp = (unsigned int *) buf;

    for (i = 0; i < count; i ++) {
        lp[i] = _aos_arc4random();
        len -= sizeof(unsigned int);
    }

    if (len > 0) {
        ranbuf = _aos_arc4random();
        _aos_memcpy(&lp[i], &ranbuf, len);
    }

    return 0;
#else
    unsigned long ranbuf, *lp;
    lp = (unsigned long *)buf;

    while (len > 0) {
        ranbuf = _aos_arc4random();
        *lp++ = ranbuf; //this op need the pointer is 4Byte-align!
        len -= sizeof(ranbuf);
    }

    return 0;
#endif
}

static u32 _aos_GetFreeHeapSize(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    int total, used, mfree = 0, peak = 0;
    aos_get_mminfo(&total, &used, &mfree, &peak);
    return mfree;
}

void *tcm_heap_malloc(int size);
static int _aos_create_task(struct task_struct *ptask, const char *name,
                            u32  stack_size, u32 priority, thread_func_t func, void *thctx)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    int ret = 0;

    if(strcmp("xmit_thread", name) == 0) {
        stack_size *= 10;
    } else if (strcmp("cmd_thread", name) == 0) {
        stack_size *= 8;
    } else {
        stack_size *= 8;
    }

    ret = aos_task_new_ext((aos_task_t*)&ptask->task, name, func, thctx, stack_size, AOS_DEFAULT_APP_PRI - priority + 8);

    ptask->task_name = name;

    if (ret != 0) {
        printf("Create Task \"%s\" Failed! ret=%d\n", ptask->task_name, ret);
    }

    DBG_TRACE("Create Task \"%s\"\n", ptask->task_name);

    if (ret == 0) {
        return _SUCCESS;    //pass
    } else {
        return _FAIL;    //fail
    }

    //return ret;
}

static void _aos_delete_task(struct task_struct *ptask)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    aos_task_exit(0);
    ptask->task = 0;

    DBG_TRACE("Delete Task \"%s\"\n", ptask->task_name);
}

static void _aos_thread_enter(char *name)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    DBG_INFO("\n\rRTKTHREAD %s\n", name);
}

static void _aos_thread_exit(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    DBG_INFO("\n\rRTKTHREAD exit %s\n", __FUNCTION__);
    aos_task_exit(0);
}


typedef struct atimer_t {
    aos_timer_t timer;
    TIMER_FUN pxCallbackFunction;
    void *arg;
} atimer;
#define TIMER_NUM 100

static atimer g_wifi_timer[TIMER_NUM] = {0};

atimer *_get_timer()
{
    int i;
    for (i = 0; i < TIMER_NUM; i++) {
        if (g_wifi_timer[i].pxCallbackFunction == NULL) {
            return &g_wifi_timer[i];
        }
    }

    return NULL;
}

void timer_callback_wrapper(void *ktimer, void *atimer_handle)
{
    atimer *timer = (atimer *)atimer_handle;

    if (!(timer && timer->pxCallbackFunction)) {
        return;
    }
    
    timer->pxCallbackFunction(timer->arg);
}

_timerHandle _aos_timerCreate(const signed char *pcTimerName,
                              osdepTickType xTimerPeriodInTicks,
                              u32 uxAutoReload,
                              void *pvTimerID,
                              TIMER_FUN pxCallbackFunction)
{
    int ret = 0;

    if (xTimerPeriodInTicks == TIMER_MAX_DELAY) {
        xTimerPeriodInTicks = 10 * 365 * 86400; // aos < v3.3.0 smaller than RHINO_MAX_TICKS, aos = v3.3.0 smaller than MAX_TIMER_TICKS
    }

    int ms = krhino_ticks_to_ms(xTimerPeriodInTicks);

    atimer *timer = _get_timer();
    timer->pxCallbackFunction = pxCallbackFunction;
    timer->arg = timer;

    ret = aos_timer_new_ext(&timer->timer, timer_callback_wrapper, timer, ms, uxAutoReload, 0);

    if (ret != 0) {
        printf("Timer create failed\n");
    }

#if DBG_OS_API
    printf("[AOS]%s handle=%x name=%s period=%d autoreload=%d callback=%x\n",
           __FUNCTION__, timer, pcTimerName, xTimerPeriodInTicks, uxAutoReload, pxCallbackFunction);
#endif

    return timer;
}

u32 _aos_timerDelete(_timerHandle xTimer,
                     osdepTickType xBlockTime)
{
    atimer *timer = (atimer *)xTimer;

#if DBG_OS_API
    printf("[AOS]%s handle=%x\n", __FUNCTION__, xTimer);
#endif
    timer->pxCallbackFunction = NULL;

    aos_timer_stop(&timer->timer);
    aos_timer_free(&timer->timer);

    return _SUCCESS;
}

u32 _aos_timerIsTimerActive(_timerHandle xTimer)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s handle=%x\n", __FUNCTION__, xTimer);
#endif

    aos_timer_t *aos_timer = (aos_timer_t *)&timer->timer;
    ktimer_t *ktimer = (ktimer_t *)(aos_timer->hdl);
    return ktimer->timer_state;
}

u32  _aos_timerStop(_timerHandle xTimer,
                    osdepTickType xBlockTime)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s handle=%x\n", __FUNCTION__, xTimer);
#endif
    aos_timer_stop(&timer->timer);
    return _SUCCESS;
}

u32  _aos_timerChangePeriod(_timerHandle xTimer,
                            osdepTickType xNewPeriod,
                            osdepTickType xBlockTime)
{
    atimer *timer = (atimer *)xTimer;
    aos_timer_t *aos_timer = (aos_timer_t *)&timer->timer;
    ktimer_t *ktimer = (ktimer_t *)(aos_timer->hdl);

#if DBG_OS_API
    printf("[AOS]%s handle=%x,xNewPeriod=%d,xBlockTime=%d\n", __FUNCTION__, xTimer, xNewPeriod, xBlockTime);
#endif

    //int old_round = ktimer->round_ticks;//save old
    //aos_timer_change(&timer->timer, xNewPeriod);
    //ktimer->round_ticks = old_round; //restore old
    //printf("tttt0 = %d\n", ((ktimer_t *)ktimer)->round_ticks);

    krhino_timer_change(ktimer, xNewPeriod, ktimer->round_ticks);

    aos_timer_start(&timer->timer);

    return _SUCCESS;
}

void *_aos_timerGetID(_timerHandle xTimer)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    atimer *timer = (atimer *)xTimer;

    //aos_timer_t *timer = (aos_timer_t *)xTimer;
    return timer;
}

u32  _aos_timerStart(_timerHandle xTimer,
                     osdepTickType xBlockTime)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s handle=%x\n", __FUNCTION__, xTimer);
#endif
    aos_timer_start(&timer->timer);
    return _SUCCESS;

}

u32  _aos_timerStartFromISR(_timerHandle xTimer,
                            osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_timer_start(&timer->timer);
    return _SUCCESS;

}

u32  _aos_timerStopFromISR(_timerHandle xTimer,
                           osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_timer_stop(&timer->timer);
    return _SUCCESS;


}

u32  _aos_timerResetFromISR(_timerHandle xTimer,
                            osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_timer_stop(&timer->timer);
    aos_timer_start(&timer->timer);

    return _SUCCESS;
}

u32  _aos_timerChangePeriodFromISR(_timerHandle xTimer,
                                   osdepTickType xNewPeriod,
                                   osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

    if (xNewPeriod == 0) {
        xNewPeriod += 1;
    }

    aos_timer_change(&timer->timer, xNewPeriod);

    return _SUCCESS;
}

u32  _aos_timerReset(_timerHandle xTimer,
                     osdepTickType xBlockTime)
{
    atimer *timer = (atimer *)xTimer;
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    aos_timer_stop(&timer->timer);
    aos_timer_start(&timer->timer);

    return _SUCCESS;
}

void _aos_acquire_wakelock()
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

}

void _aos_release_wakelock()
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

}

void _aos_wakelock_timeout(uint32_t timeout)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif

}

u8 _aos_get_scheduler_state(void)
{
#if DBG_OS_API
    printf("[AOS]%s\n", __FUNCTION__);
#endif
    extern kstat_t  g_sys_stat;
    kstat_t get = g_sys_stat;

    if (get == RHINO_STOPPED) {
        return OS_SCHEDULER_SUSPENDED;
    } else if (get == RHINO_RUNNING) {
        return OS_SCHEDULER_RUNNING;
    } else {
        printf("Error %s\n", __FUNCTION__);
        return OS_SCHEDULER_RUNNING;
    }
}


const struct osdep_service_ops osdep_service = {
    _aos_malloc,            //rtw_vmalloc
    _aos_zmalloc,           //rtw_zvmalloc
    _aos_mfree,         //rtw_vmfree
    _aos_malloc,            //rtw_malloc
    _aos_zmalloc,           //rtw_zmalloc
    _aos_mfree,         //rtw_mfree
    _aos_memcpy,            //rtw_memcpy
    _aos_memcmp,            //rtw_memcmp
    _aos_memset,            //rtw_memset
    _aos_init_sema,     //rtw_init_sema
    _aos_free_sema,     //rtw_free_sema
    _aos_up_sema,           //rtw_up_sema
    _aos_up_sema_from_isr,  //rtw_up_sema_from_isr
    _aos_down_sema,     //rtw_down_sema
    _aos_mutex_init,        //rtw_mutex_init
    _aos_mutex_free,        //rtw_mutex_free
    _aos_mutex_get,     //rtw_mutex_get
    _aos_mutex_get_timeout,//rtw_mutex_get_timeout
    _aos_mutex_put,     //rtw_mutex_put
    _aos_enter_critical,    //rtw_enter_critical
    _aos_exit_critical, //rtw_exit_critical
    _aos_enter_critical_from_isr,   //rtw_enter_critical_from_isr
    _aos_exit_critical_from_isr,    //rtw_exit_critical_from_isr
    _aos_enter_critical,        //rtw_enter_critical_bh
    _aos_exit_critical,     //rtw_exit_critical_bh
    _aos_enter_critical_mutex,  //rtw_enter_critical_mutex
    _aos_exit_critical_mutex,   //rtw_exit_critical_mutex
    _aos_cpu_lock,      //rtw_cpu_lock
    _aos_cpu_unlock,        //rtw_cpu_unlock
    _aos_spinlock_init,     //rtw_spinlock_init
    _aos_spinlock_free,     //rtw_spinlock_free
    _aos_spinlock,              //rtw_spin_lock
    _aos_spinunlock,            //rtw_spin_unlock
    _aos_spinlock_irqsave,      //rtw_spinlock_irqsave
    _aos_spinunlock_irqsave,    //rtw_spinunlock_irqsave
    _aos_init_xqueue,           //rtw_init_xqueue
    _aos_push_to_xqueue,        //rtw_push_to_xqueue
    _aos_pop_from_xqueue,       //rtw_pop_from_xqueue
    _aos_deinit_xqueue,     //rtw_deinit_xqueue
    _aos_get_current_time,      //rtw_get_current_time
    _aos_systime_to_ms,     //rtw_systime_to_ms
    _aos_systime_to_sec,        //rtw_systime_to_sec
    _aos_ms_to_systime,     //rtw_ms_to_systime
    _aos_sec_to_systime,        //rtw_sec_to_systime
    _aos_msleep_os, //rtw_msleep_os
    _aos_usleep_os, //rtw_usleep_os
    _aos_mdelay_os, //rtw_mdelay_os
    _aos_udelay_os, //rtw_udelay_os
    _aos_yield_os,      //rtw_yield_os

    _aos_ATOMIC_SET,    //ATOMIC_SET
    _aos_ATOMIC_READ,   //ATOMIC_READ
    _aos_ATOMIC_ADD,    //ATOMIC_ADD
    _aos_ATOMIC_SUB,    //ATOMIC_SUB
    _aos_ATOMIC_INC,    //ATOMIC_INC
    _aos_ATOMIC_DEC,    //ATOMIC_DEC
    _aos_ATOMIC_ADD_RETURN, //ATOMIC_ADD_RETURN
    _aos_ATOMIC_SUB_RETURN, //ATOMIC_SUB_RETURN
    _aos_ATOMIC_INC_RETURN, //ATOMIC_INC_RETURN
    _aos_ATOMIC_DEC_RETURN, //ATOMIC_DEC_RETURN

    _aos_modular64,         //rtw_modular64
    _aos_get_random_bytes,      //rtw_get_random_bytes
    _aos_GetFreeHeapSize,       //rtw_getFreeHeapSize

    _aos_create_task,           //rtw_create_task
    _aos_delete_task,           //rtw_delete_task
    NULL,                           //rtw_wakeup_task

    _aos_thread_enter,          //rtw_thread_enter
    _aos_thread_exit,           //rtw_thread_exit

    _aos_timerCreate,           //rtw_timerCreate,
    _aos_timerDelete,           //rtw_timerDelete,
    _aos_timerIsTimerActive,    //rtw_timerIsTimerActive,
    _aos_timerStop,         //rtw_timerStop,
    _aos_timerChangePeriod, //rtw_timerChangePeriod
    _aos_timerGetID,            //rtw_timerGetID
    _aos_timerStart,            //rtw_timerStart
    _aos_timerStartFromISR, //rtw_timerStartFromISR
    _aos_timerStopFromISR,      //rtw_timerStopFromISR
    _aos_timerResetFromISR, //rtw_timerResetFromISR
    _aos_timerChangePeriodFromISR,  //rtw_timerChangePeriodFromISR
    _aos_timerReset,            //rtw_timerReset

    _aos_acquire_wakelock,      //rtw_acquire_wakelock
    _aos_release_wakelock,      //rtw_release_wakelock
    _aos_wakelock_timeout,      //rtw_wakelock_timeout
    _aos_get_scheduler_state    //rtw_get_scheduler_state
};


#if 0
void TIMER_FUN1(void *context)
{
	//printf("TIMER_FUN\n");
    timer_test();
    printf("Curtime=%d\n", osdep_service.rtw_get_current_time());
}

void timer_test() {

    int test_systime = 1000;
    int test_ms = 1000;
    printf("rtw_ms_to_systime(%d)=%d, rtw_systime_to_ms(%d)=%d\n", 
        test_ms, osdep_service.rtw_ms_to_systime(test_ms),
        test_systime, osdep_service.rtw_systime_to_ms(test_systime));

    
    printf("timer_test\n");
    int timer_period = 1000;
    int ticks = osdep_service.rtw_ms_to_systime(timer_period);
    _timerHandle handle = osdep_service.rtw_timerCreate("Test1", ticks, 0, NULL, TIMER_FUN1);
    osdep_service.rtw_timerChangePeriod(handle, ticks, -1);
}
#endif


