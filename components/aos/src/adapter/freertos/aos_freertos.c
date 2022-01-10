/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>
#include <event_groups.h>
#include <aos/aos.h>
#include <drv/irq.h>

// weak hook
__attribute__((weak)) void aos_task_create_hook_lwip_thread_sem(aos_task_t *task)
{

}

__attribute__((weak)) void aos_task_del_hook_lwip_thread_sem(aos_task_t *task, void *arg)
{
    
}
volatile int g_intrpt_nested_cnt;
static uint32_t is_in_intrp(void)
{
    return g_intrpt_nested_cnt;
}

void aos_reboot_ext(int cmd)
{
    extern void drv_reboot(int cmd);
    drv_reboot(cmd);
}

void aos_reboot(void)
{
    aos_reboot_ext(0);
}

int aos_get_hz(void)
{
    return configTICK_RATE_HZ;
}

const char *aos_version_get(void)
{
    return aos_get_os_version();
}


#define AOS_MAGIC 0x20171020
typedef struct {
    uint32_t key_bitmap;
    void *keys[4];
    void *stack;
    char name[32];
    uint32_t magic;
} AosStaticTask_t;

struct targ {
    AosStaticTask_t *task;
    void (*fn)(void *);
    void *arg;
};

static void dfl_entry(void *arg)
{
    struct targ *targ = arg;
    void (*fn)(void *) = targ->fn;
    void *farg = targ->arg;
    free(targ);

    fn(farg);

    vTaskDelete(NULL);
}

void vPortCleanUpTCB(void *pxTCB)
{
    AosStaticTask_t *task = (AosStaticTask_t *)pxTCB;

    if (task->magic != AOS_MAGIC) {
        return;
    }

    free(task->stack);
    free(task);
}

int aos_task_new(const char *name, void (*fn)(void *), void *arg,
                 int stack_size)
{

    return aos_task_new_ext(NULL,name,fn,arg,stack_size,AOS_DEFAULT_APP_PRI);
}

int aos_task_new_ext(aos_task_t *task, const char *name, void (*fn)(void *), void *arg,
                     int stack_size, int prio)
{
    TaskHandle_t xHandle;
    /* vreify param */
    if (fn == NULL || (stack_size % 4 != 0) ) {
        return -1;
    }

    /*create task */
    if (name == NULL) {
        name = "default_name";
    }
    BaseType_t ret = xTaskCreate(fn, name, stack_size>>2, arg, configMAX_PRIORITIES-prio, &xHandle);

    if (ret == pdPASS) {
        if(task) {
            task->hdl = xHandle;
        }
        aos_task_create_hook_lwip_thread_sem(task);
        return 0;
    } else {
        return -1;
    }

    return 0;
}

void aos_task_exit(int code)
{
    /* task exit by itself */
    vTaskDelete(NULL);
    aos_task_t task = aos_task_self();
// #if (RHINO_CONFIG_USER_HOOK_FOR_LWIP > 0)
    aos_task_del_hook_lwip_thread_sem(&task, NULL);
// #endif
}

aos_status_t aos_task_delete(aos_task_t *task)
{
    TaskHandle_t *ptask = (TaskHandle_t *)task;

    vTaskDelete(ptask);

    return 0;
}
const char *aos_task_name(void)
{
    TaskHandle_t *task = (TaskHandle_t *)xTaskGetCurrentTaskHandle();
    return  pcTaskGetTaskName(task);

}
const char *aos_task_get_name(aos_task_t *task)
{
    TaskHandle_t *ptask = (TaskHandle_t *)task;

    return pcTaskGetTaskName(ptask);
}


void aos_task_show_info(void)
{
    uint32_t total_task_number = 0;
    uint32_t i;
    uint32_t total_time;
    TaskStatus_t *pTaskStatus = NULL, *pTaskStatus_bk = NULL;
    total_task_number = uxTaskGetNumberOfTasks();
    pTaskStatus = (TaskStatus_t*)aos_malloc(sizeof(TaskStatus_t)* total_task_number);
    pTaskStatus_bk = pTaskStatus;
    if(pTaskStatus == NULL) {
        return;
    }

    total_task_number = uxTaskGetSystemState(pTaskStatus,total_task_number,&total_time);
    printf("task                 pri status  stack_wm      \n");
    printf("-------------------- --- ------- ------------- \n");


    for (i = 0; i < total_task_number; i++) {

        printf("%-20s %3d ", pTaskStatus->pcTaskName, (uint32_t)(pTaskStatus->uxCurrentPriority));

        if (pTaskStatus->eCurrentState == eRunning)            printf("runing ");
        else if ( pTaskStatus->eCurrentState == eReady)        printf("ready  ");
        else if ( pTaskStatus->eCurrentState == eBlocked)      printf("blocked");
        else if ( pTaskStatus->eCurrentState == eSuspended)    printf("suspend");
        else if ( pTaskStatus->eCurrentState == eDeleted)      printf("deleted");
        else
            printf("%-7d", pTaskStatus->eCurrentState);

        printf(" %-13d \n",pTaskStatus->usStackHighWaterMark);
        pTaskStatus++;
    }

    aos_free(pTaskStatus_bk);
}

int aos_task_key_create(aos_task_key_t *key)
{
    //todo
    /*
    AosStaticTask_t *task = (AosStaticTask_t *)xTaskGetCurrentTaskHandle();
    int i;

    if (task->magic != AOS_MAGIC)
        return -1;

    for (i=0;i<4;i++) {
        if (task->key_bitmap & (1 << i))
            continue;

        task->key_bitmap |= 1 << i;
        *key = i;
        return 0;
    }
    */
    return -1;
}

void aos_task_key_delete(aos_task_key_t key)
{
    //todo
    /*
    AosStaticTask_t *task = (AosStaticTask_t *)xTaskGetCurrentTaskHandle();
    if (task->magic != AOS_MAGIC)
        return;
    task->key_bitmap &= ~(1 << key);
    */
}

int aos_task_setspecific(aos_task_key_t key, void *vp)
{
    AosStaticTask_t *task = (AosStaticTask_t *)xTaskGetCurrentTaskHandle();

	vTaskSetThreadLocalStoragePointer(task, key, vp);

    return 0;
}

void *aos_task_getspecific(aos_task_key_t key)
{
    AosStaticTask_t *task = (AosStaticTask_t *)xTaskGetCurrentTaskHandle();
    
    return (void *)pvTaskGetThreadLocalStoragePointer(task, key);;
}

void aos_task_wdt_attach(void (*will)(void *), void *args)
{
#ifdef CONFIG_SOFTWDT
    aos_wdt_attach((uint32_t)xTaskGetCurrentTaskHandle(), will, args);
#else
    (void)will;
    (void)args;
#endif
}

void aos_task_wdt_detach()
{
#ifdef CONFIG_SOFTWDT
    uint32_t index = (uint32_t)xTaskGetCurrentTaskHandle();

    aos_wdt_feed(index, 0);
    aos_wdt_detach(index);
#endif
}

void aos_task_wdt_feed(int time)
{
#ifdef CONFIG_SOFTWDT
    ktask_t *task = xTaskGetCurrentTaskHandle();

    if (!aos_wdt_exists((uint32_t)task))
        aos_wdt_attach((uint32_t)task, NULL, (void*)task->task_name);

    aos_wdt_feed((uint32_t)task, time);
#endif
}

int aos_mutex_new(aos_mutex_t *mutex)
{
    SemaphoreHandle_t mux = xSemaphoreCreateMutex();
    mutex->hdl = mux;
    return mux != NULL ? 0 : -1;
}

void aos_mutex_free(aos_mutex_t *mutex)
{
    vSemaphoreDelete(mutex->hdl);
}

int aos_mutex_lock(aos_mutex_t *mutex, unsigned int ms)
{
    if (mutex && mutex->hdl) {
        if(is_in_intrp()) {
            BaseType_t temp = pdFALSE;
            xSemaphoreTakeFromISR(mutex->hdl,&temp);
        } else {
            xSemaphoreTake(mutex->hdl, ms == AOS_WAIT_FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(ms));
        }
    }
    return 0;
}

int aos_mutex_unlock(aos_mutex_t *mutex)
{
    if (mutex && mutex->hdl) {
        if(is_in_intrp()) {
            BaseType_t temp = pdFALSE;
            xSemaphoreGiveFromISR(mutex->hdl,&temp);
        } else {
            xSemaphoreGive(mutex->hdl);
        }
    }
    return 0;
}

int aos_mutex_is_valid(aos_mutex_t *mutex)
{
    return mutex->hdl != NULL;
}

int aos_sem_new(aos_sem_t *sem, int count)
{
    SemaphoreHandle_t s = xSemaphoreCreateCounting(1024, count);
    sem->hdl = s;
    return 0;
}

void aos_sem_free(aos_sem_t *sem)
{
    if (sem == NULL || sem->hdl ) {
        return;
    }

    vSemaphoreDelete(sem->hdl);
}

int aos_sem_wait(aos_sem_t *sem, unsigned int ms)
{
    if (sem == NULL) {
        return -1;
    }
    int ret;
    if(is_in_intrp()) {
        BaseType_t pxHiProTskWkup = pdTRUE;
        ret = xSemaphoreTakeFromISR(sem->hdl, &pxHiProTskWkup);
    } else {
        ret = xSemaphoreTake(sem->hdl, ms == AOS_WAIT_FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(ms));
    }
    return ret == pdPASS ? 0 : -1;
}

void aos_sem_signal(aos_sem_t *sem)
{
    if (sem == NULL && sem->hdl) {
        return;
    }
    if(is_in_intrp()) {
        BaseType_t  temp = pdTRUE;
        xSemaphoreGiveFromISR(sem->hdl, &temp);
    } else {
        xSemaphoreGive(sem->hdl);
    }
}

int aos_sem_is_valid(aos_sem_t *sem)
{
    return sem && sem->hdl != NULL;
}

void aos_sem_signal_all(aos_sem_t *sem)
{
    //todo freertos doesn't has signall all function
    aos_sem_signal(sem);
}

int aos_queue_new(aos_queue_t *queue, void *buf, size_t size, int max_msg)
{
    xQueueHandle q;
    (void)(buf);
    /* verify param */
    if(queue == NULL || size == 0) {
        return -1;
    }

    /* create queue object */
    q = xQueueCreate(size / max_msg, max_msg);
    if(q == NULL) {
        return -1;
    }
    queue->hdl = q;

    return 0;
}

void aos_queue_free(aos_queue_t *queue)
{
    /* delete queue object */
    if(queue && queue->hdl) {
        vQueueDelete(queue->hdl);
    }

    return;
}

int aos_queue_send(aos_queue_t *queue, void *msg, unsigned int size)
{
    /* verify param */
    if(queue == NULL || msg == NULL || size == 0 ) {
        return -1;
    }

    /* send msg  to specific queue */
    return xQueueSend(queue->hdl, msg, portMAX_DELAY) == pdPASS ? 0 : -1;
}

int aos_queue_recv(aos_queue_t *queue, unsigned int ms, void *msg, size_t *size)
{
    /* verify param */
    if(queue == NULL || msg == NULL || size == 0 ) {
        return -1;
    }

    /* receive msg from specific queue */
    return xQueueReceive(queue->hdl, msg, ms == AOS_WAIT_FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(ms)) == pdPASS ? 0 : -1;

}

int aos_queue_is_valid(aos_queue_t *queue)
{
    return queue && queue->hdl != NULL;
}

void *aos_queue_buf_ptr(aos_queue_t *queue)
{
    //todo  freertos doesn't support this feature
    (void)queue;
    return NULL;
}

int aos_queue_get_count(aos_queue_t *queue)
{
    BaseType_t ret;
    ret = uxQueueMessagesWaiting(queue->hdl);
    return ret;
}

typedef struct tmr_adapter {
    TimerHandle_t timer;
    void (*func)(void *, void *);
    void *func_arg;
    uint8_t bIsRepeat;
} tmr_adapter_t;

static void tmr_adapt_cb(TimerHandle_t xTimer)
{
    tmr_adapter_t *pTmrObj = (tmr_adapter_t *)pvTimerGetTimerID(xTimer);

    if(pTmrObj->func)
        pTmrObj->func(pTmrObj,pTmrObj->func_arg);
    return;
}


int aos_timer_new(aos_timer_t *timer, void (*fn)(void *, void *),
                  void *arg, int ms, int repeat)
{
    return aos_timer_new_ext(timer,fn,arg,ms,repeat,1);
}

int aos_timer_new_ext(aos_timer_t *timer, void (*fn)(void *, void *),
                      void *arg, int ms, int repeat, unsigned char auto_run)
{
    /* verify param */
    if (timer == NULL || ms == 0 || fn == NULL) {
        return -1;
    }

    /* create timer wrap object ,then initlize timer object */
    tmr_adapter_t *tmr_adapter = pvPortMalloc(sizeof(tmr_adapter_t));

    if (tmr_adapter == NULL) {
        return -1;
    }

    tmr_adapter->func = fn;
    tmr_adapter->func_arg = arg;
    tmr_adapter->bIsRepeat = repeat;

    /* create timer by kernel api */
    TimerHandle_t ptimer = xTimerCreate("Timer", pdMS_TO_TICKS(ms),repeat,tmr_adapter, tmr_adapt_cb);

    if (timer == NULL) {
        vPortFree(tmr_adapter);
        return -1;
    }

    tmr_adapter->timer = ptimer;
    timer->hdl = (void*)tmr_adapter;

    /* start timer if auto_run == TRUE */
    if(auto_run) {
        if(aos_timer_start(timer) != 0) {
            return -1;
        }
    }

    return 0;
}

void aos_timer_free(aos_timer_t *timer)
{
    if (timer == NULL) {
        return;
    }

    tmr_adapter_t *tmr_adapter = timer->hdl;
    int ret = xTimerDelete(tmr_adapter->timer, 0);

    if (!ret) {
        return ;
    }

    vPortFree(tmr_adapter);
    timer->hdl = NULL;

    return ;
}

int aos_timer_start(aos_timer_t *timer)
{
    /* verify param */
    if (timer == NULL ) {
        return -1;
    }

    /* start timer  */
    tmr_adapter_t *tmr_adapter = timer->hdl;
    int ret;

    if(is_in_intrp()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        ret = xTimerStartFromISR(tmr_adapter->timer, &xHigherPriorityTaskWoken);
    } else {
        ret = xTimerStart(tmr_adapter->timer, 0);
    }
    if (ret != pdPASS) {
        return -1;
    }

    return 0;
}

int aos_timer_stop(aos_timer_t *timer)
{
    /* verify param */
    if (timer == NULL) {
        return -1;
    }

    /* stop timer */
    tmr_adapter_t *tmr_adapter = timer->hdl;
    int ret;

    if(is_in_intrp()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        ret = xTimerStopFromISR(tmr_adapter->timer, &xHigherPriorityTaskWoken);
    } else {
        ret = xTimerStop(tmr_adapter->timer, 0);
    }

    if (ret != pdPASS) {
        return -1;
    }

    return 0;
}

int aos_timer_change(aos_timer_t *timer, int ms)
{
    /* verify param */
    if(timer == NULL ) {
        return -1;
    }
    BaseType_t  xHigherProTskWoken  = pdFALSE;
    /* change timer period value */
    tmr_adapter_t *pTimer = timer->hdl;
    int ret;
    if(is_in_intrp()) {
        ret = xTimerChangePeriodFromISR(pTimer->timer,pdMS_TO_TICKS(ms),&xHigherProTskWoken);
    } else {
        ret = xTimerChangePeriod(pTimer->timer,pdMS_TO_TICKS(ms),10);
    }

    if(ret != pdPASS) {
        return -1;
    }

    return 0;
}

int aos_timer_is_valid(aos_timer_t *timer)
{
    tmr_adapter_t *pTimer = timer->hdl;
    if( xTimerIsTimerActive(pTimer->timer) != pdFALSE ) {
        // pTimer is active, do something.
        return 1;
    }
    else {
        // pTimer is not active, do something else.
        return 0;
    }
}

int aos_timer_change_once(aos_timer_t *timer, int ms)
{
    int ret = -1;

    aos_check_return_einval(timer && timer->hdl);
    tmr_adapter_t *pTimer = timer->hdl;
    if( xTimerIsTimerActive(pTimer->timer) != pdFALSE ) {
        xTimerStop(pTimer->timer, 100);
    }
    if( xTimerIsTimerActive(pTimer->timer) == pdFALSE ) {

        if(is_in_intrp()) {
            BaseType_t xHigherProTskWoken = pdFALSE;
            ret = xTimerChangePeriodFromISR(pTimer->timer,pdMS_TO_TICKS(ms),&xHigherProTskWoken);
        } else {
            ret = xTimerChangePeriod(pTimer->timer, pdMS_TO_TICKS(ms), 100);
        }

        if (ret == pdPASS) {
            return 0;
        } else {
            return -1;
        }
    }

    return ret;
}

int aos_workqueue_create(aos_workqueue_t *workqueue, int pri, int stack_size)
{
    //todo
    return 0;
}

void aos_workqueue_del(aos_workqueue_t *workqueue)
{
    //todo
    return;
}

struct work {
    void (*fn)(void *);
    void *arg;
    int dly;
};

int aos_work_init(aos_work_t *work, void (*fn)(void *), void *arg, int dly)
{
    //todo
    struct work *w = malloc(sizeof(*w));
    w->fn = fn;
    w->arg = arg;
    w->dly = dly;
    work->hdl = w;
    return 0;
}

void aos_work_destroy(aos_work_t *work)
{
    //todo
    free(work->hdl);
}

int aos_work_run(aos_workqueue_t *workqueue, aos_work_t *work)
{
    //todo
    return aos_work_sched(work);
}

static void worker_entry(void *arg)
{
    //todo
    struct work *w = arg;
    if (w->dly) {
        usleep(w->dly * 1000);
    }
    w->fn(w->arg);
}

int aos_work_sched(aos_work_t *work)
{
    //todo
    struct work *w = work->hdl;
    return aos_task_new("worker", worker_entry, w, 8192);
}

int aos_work_cancel(aos_work_t *work)
{
    //todo
    return -1;
}

void *yoc_malloc(int32_t size, void *caller)
{
    (void)caller;
    return pvPortMalloc(size);

}

void *yoc_realloc(void *ptr, size_t size, void *caller)
{
    (void)caller;
    return pvPortRealloc(ptr,size);

}

void yoc_free(void *ptr, void *caller)
{
    (void)caller;
    vPortFree(ptr);
}
void *aos_zalloc(size_t size)
{
    void* ptr = pvPortMalloc(size);
    if(ptr) {
        bzero(ptr,size);
    }
    return ptr;
}

void *aos_malloc(size_t size)
{
    return pvPortMalloc(size);
}

void *aos_realloc(void *mem, size_t size)
{
    return pvPortRealloc(mem,size);
}


void aos_alloc_trace(void *addr, size_t allocator)
{
    //todo
}

void aos_free(void *mem)
{
    vPortFree(mem);
}

void *aos_zalloc_check(size_t size)
{
    void *ptr = yoc_malloc(size, __builtin_return_address(0));

    aos_check_mem(ptr);
    if (ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

void *aos_malloc_check(size_t size)
{
    void *p = yoc_malloc(size, __builtin_return_address(0));
    aos_check_mem(p);

    return p;
}

void *aos_calloc_check(size_t size, size_t num)
{
    return aos_zalloc_check(size * num);
}

void *aos_realloc_check(void *ptr, size_t size)
{
    void *new_ptr = yoc_realloc(ptr, size, __builtin_return_address(0));
    aos_check_mem(new_ptr);

    return new_ptr;
}

//static struct timeval sys_start_time;
long long aos_now(void)
{
    return aos_now_ms()*1000*1000;
}

long long aos_now_ms(void)
{
    long long ms;
    uint32_t tick_cnt = xTaskGetTickCount();
    ms = tick_cnt * 1000 / configTICK_RATE_HZ;
    return ms;
}

void aos_msleep(int ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
    return;
}

void aos_init(void)
{
    return;
}

void aos_start(void)
{
    vTaskStartScheduler();
}

void dumpsys_task_func(void)
{
#if configUSE_TRACE_FACILITY
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime, ulStatsAsPercentage;

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = malloc( uxArraySize * sizeof( TaskStatus_t ) );
    if( pxTaskStatusArray == NULL )
        return;

    uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );
    // For percentage calculations.
    ulTotalRunTime /= 100UL;

    // Avoid divide by zero errors.
    if( ulTotalRunTime > 0 ) {
        // For each populated position in the pxTaskStatusArray array,
        // format the raw data as human readable ASCII data
        for( x = 0; x < uxArraySize; x++ ) {
            // What percentage of the total run time has the task used?
            // This will always be rounded down to the nearest integer.
            // ulTotalRunTimeDiv100 has already been divided by 100.
            ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

            if( ulStatsAsPercentage > 0UL ) {
                printf("%s\t\t%u\t\t%u%%\r\n", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage );
            } else {
                printf("%s\t\t%u\t\t<1%%\r\n", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter );
            }
        }
    }

    free( pxTaskStatusArray );
#endif
}

int32_t aos_irq_context(void)
{
    return is_in_intrp();
}

void aos_task_yield()
{
    taskYIELD();
}

aos_task_t aos_task_self(void)
{
    static aos_task_t task;
    task.hdl = xTaskGetCurrentTaskHandle();

    return task;
}

int aos_event_new(aos_event_t *event, unsigned int flags)
{
    EventGroupHandle_t event_handle;
    event->hdl = NULL;
    aos_check_return_einval(event);

    /* create event handle */
    event_handle = xEventGroupCreate();
    /* initlized event */
    if(event_handle != NULL) {
        xEventGroupSetBits(event_handle,flags);
    } else {
        return -1;
    }
    event->hdl = event_handle;

    return 0;
}

void aos_event_free(aos_event_t *event)
{
    aos_check_return(event && event->hdl);

    vEventGroupDelete((EventGroupHandle_t)event->hdl);

    event->hdl = NULL;
}

int aos_event_get
(
    aos_event_t *event,
    unsigned int flags,
    unsigned char opt,
    unsigned int *actl_flags,
    unsigned int timeout
)
{
    uint32_t   wait_bits = 0;
    (void)opt;
    aos_check_return_einval(event && event->hdl);

    if (timeout == AOS_WAIT_FOREVER) {
        wait_bits = xEventGroupWaitBits(event->hdl,
                                        flags,
                                        pdTRUE,
                                        pdFALSE,
                                        0xffffffff
                                       );
    } else {
        wait_bits=  xEventGroupWaitBits(event->hdl,
                                        flags,
                                        pdTRUE,
                                        pdFALSE,
                                        pdMS_TO_TICKS(timeout)
                                       );
    }

    *actl_flags = wait_bits;
    return 0;
}

int aos_event_set(aos_event_t *event, unsigned int flags, unsigned char opt)
{
    aos_check_return_einval(event && event->hdl);
    if(is_in_intrp()) {
        BaseType_t xHighProTaskWoken = pdFALSE;
        xEventGroupSetBitsFromISR(event->hdl,flags,&xHighProTaskWoken);
    } else {
        xEventGroupSetBits(event->hdl,flags);
    }

    return 0;
}

int aos_event_is_valid(aos_event_t *event)
{
    EventGroupHandle_t k_event;

    if (event == NULL) {
        return 0;
    }

    k_event = event->hdl;

    if (k_event == NULL) {
        return 0;
    }

    return 1;
}

/// Suspend the scheduler.
/// \return time in ticks, for how long the system can sleep or power-down.
void aos_kernel_sched_suspend(void)
{
    vTaskSuspendAll();
}

/// Resume the scheduler.
/// \param[in]     sleep_ticks   time in ticks for how long the system was in sleep or power-down mode.
void aos_kernel_sched_resume()
{
    xTaskResumeAll();
}

/* YoC extend aos API */

int aos_get_mminfo(int32_t *total, int32_t *used, int32_t *mfree, int32_t *peak)
{
    aos_check_return_einval(total && used && mfree && peak);
    //todo
    *total = 0;
    *used =  0;
    *mfree = 0;
    *peak =  0;

    return 0;
}

int aos_mm_dump(void)
{
#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_MM)
//todo
#endif
    return 0;
}

uint64_t aos_kernel_tick2ms(uint32_t ticks)
{
    return ticks*1000/configTICK_RATE_HZ;
}

uint64_t aos_kernel_ms2tick(uint32_t ms)
{
    return ms *configTICK_RATE_HZ/1000;
}

k_status_t aos_kernel_intrpt_enter(void)
{
    g_intrpt_nested_cnt ++;
    return 0;
}

k_status_t aos_kernel_intrpt_exit(void)
{
    g_intrpt_nested_cnt --;
    portYIELD_FROM_ISR(pdTRUE);
    return 0;
}
