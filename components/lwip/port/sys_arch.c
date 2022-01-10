/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* system includes */
#include <aos/kernel.h>
#include <aos/list.h>
#include <aos/aos.h>
/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "arch/sys_arch.h"
#include <aos/aos.h>
#include <k_api.h>

static aos_mutex_t sys_arch_mutex;

/*-----------------------------------------------------------------------------------*/
/**
*  @brief Creates a new semaphore.
*  @param sem   sem struct pointer
*  @param count
*  @return   0  OK
*           -1  ERR
*/
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    err_t ret = ERR_MEM;
    int stat = aos_sem_new(sem, count);

    if (stat == 0) {
        ret = ERR_OK;
    }

    return ret;
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief Deallocates a semaphore.
*  @param sem   sem struct
*  @return null
*/
void sys_sem_free(sys_sem_t *sem)
{
    if ((sem != NULL)) {
        aos_sem_free(sem);
    }
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief Signals a semaphore.
*  @param sem   sem struct pointer
*  @return null
*/
void sys_sem_signal(sys_sem_t *sem)
{
    aos_sem_signal(sem);
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief Blocks the thread while waiting for the semaphore to be
          signaled. If the "timeout" argument is non-zero, the thread should
          only be blocked for the specified time (measured in
          milliseconds).

          If the timeout argument is non-zero, the return value is the number of
          milliseconds spent waiting for the semaphore to be signaled. If the
          semaphore wasn't signaled within the specified time, the return value is
          SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
          (i.e., it was already signaled), the function may return zero.

          Notice that lwIP implements a function with a similar name,
          sys_sem_wait(), that uses the sys_arch_sem_wait() function.
*  @param sem     sem struct pointer
*  @param timeout
*  @return  has been waited time
*/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    u32_t begin_ms, end_ms, elapsed_ms;
    u32_t ret;

    if (sem == NULL) {
        return SYS_ARCH_TIMEOUT;
    }

    begin_ms = sys_now();

    if (timeout != 0UL) {
        ret = aos_sem_wait(sem, timeout);

        if (ret == 0) {
            end_ms = sys_now();

            elapsed_ms = end_ms - begin_ms;

            ret = elapsed_ms;
        } else {
            ret = SYS_ARCH_TIMEOUT;
        }
    } else {
        while (!(aos_sem_wait(sem, AOS_WAIT_FOREVER) == 0));

        end_ms = sys_now();

        elapsed_ms = end_ms - begin_ms;

        if (elapsed_ms == 0UL) {
            elapsed_ms = 1UL;
        }

        ret = elapsed_ms;
    }

    return ret;
}

#ifndef LWIP_MAILBOX_QUEUE
/*-----------------------------------------------------------------------------------*/
/**
*  @brief Creates an empty mailbox for maximum "size" elements.
*  @param mb     mbox struct pointer
*  @param size
*  @return  0  OK
*          -1  ERR
*/
err_t sys_mbox_new(sys_mbox_t *mb, int size)
{
    struct sys_mbox *mbox;
    LWIP_UNUSED_ARG(size);

    mbox = (struct sys_mbox *)malloc(sizeof(struct sys_mbox));

    if (mbox == NULL) {
        return ERR_MEM;
    }

    memset(mbox, 0, sizeof(struct sys_mbox));

    mbox->first = mbox->last = 0;
    sys_sem_new(&mbox->not_empty, 0);
    sys_sem_new(&mbox->not_full, 0);
    sys_sem_new(&mbox->mutex, 1);
    mbox->wait_send = 0;

    *mb = mbox;
    return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief     Deallocates a mailbox. If there are messages still present in the
              mailbox when the mailbox is deallocated, it is an indication of a
              programming error in lwIP and the developer should be notified.
*  @param mb  mbox struct pointer
*  @return    null
*/
void sys_mbox_free(sys_mbox_t *mb)
{
    if ((mb != NULL) && (*mb != SYS_MBOX_NULL)) {
        struct sys_mbox *mbox = *mb;

        sys_arch_sem_wait(&mbox->mutex, 0);

        sys_sem_free(&mbox->not_empty);
        sys_sem_free(&mbox->not_full);
        sys_sem_free(&mbox->mutex);
        /*  LWIP_DEBUGF("sys_mbox_free: mbox 0x%lx\n", mbox); */
        free(mbox);
    }
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief Posts the "msg" to the mailbox. This function have to block until  the "msg" is really posted.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @return  null
*/
void sys_mbox_post(sys_mbox_t *mb, void *msg)
{
    u8_t first;
    struct sys_mbox *mbox;
    LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
    mbox = *mb;

    sys_arch_sem_wait(&mbox->mutex, 0);

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox %p msg %p\n", (void *)mbox, (void *)msg));

    while ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
        mbox->wait_send++;
        sys_sem_signal(&mbox->mutex);
        sys_arch_sem_wait(&mbox->not_full, 0);
        sys_arch_sem_wait(&mbox->mutex, 0);
        mbox->wait_send--;
    }

    mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;

    if (mbox->last == mbox->first) {
        first = 1;
    } else {
        first = 0;
    }

    mbox->last++;

    if (first) {
        sys_sem_signal(&mbox->not_empty);
    }

    sys_sem_signal(&mbox->mutex);
}

/**
*  @brief Try to post the "msg" to the mailbox. Returns ERR_MEM if this one  is full, else, ERR_OK if the "msg" is posted.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @return  0  OK
*          -1  ERR
*/
err_t sys_mbox_trypost(sys_mbox_t *mb, void *msg)
{
    u8_t first;
    struct sys_mbox *mbox;
    LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
    mbox = *mb;

    sys_arch_sem_wait(&mbox->mutex, 0);

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox %p msg %p\n",
                            (void *)mbox, (void *)msg));

    if ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
        sys_sem_signal(&mbox->mutex);
        return ERR_MEM;
    }

    mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;

    if (mbox->last == mbox->first) {
        first = 1;
    } else {
        first = 0;
    }

    mbox->last++;

    if (first) {
        sys_sem_signal(&mbox->not_empty);
    }

    sys_sem_signal(&mbox->mutex);
    return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/*
   @brief     Blocks the thread until a message arrives in the mailbox, but does
              not block the thread longer than "timeout" milliseconds (similar to
              the sys_arch_sem_wait() function). The "msg" argument is a result
              parameter that is set by the function (i.e., by doing "*msg =
              ptr"). The "msg" parameter maybe NULL to indicate that the message
              should be dropped.

              The return values are the same as for the sys_arch_sem_wait() function:
              Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
              timeout.

              Note that a function with a similar name, sys_mbox_fetch(), is
              implemented by lwIP.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @param timeout  fetch msg in time
*  @return  time has wait
*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mb, void **msg, u32_t timeout)
{
    u32_t time_needed = 0;
    struct sys_mbox *mbox;
    LWIP_ASSERT("invalid mbox", (mb != NULL));
    mbox = *mb;

    /* The mutex lock is quick so we don't bother with the timeout
     stuff here. */
    sys_arch_sem_wait(&mbox->mutex, 0);

    while (mbox->first == mbox->last) {
        sys_sem_signal(&mbox->mutex);

        /* We block while waiting for a mail to arrive in the mailbox. We
           must be prepared to timeout. */
        if (timeout != 0) {
            time_needed = sys_arch_sem_wait(&mbox->not_empty, timeout);

            if (time_needed == SYS_ARCH_TIMEOUT) {
                return SYS_ARCH_TIMEOUT;
            }
        } else {
            sys_arch_sem_wait(&mbox->not_empty, 0);
        }

        sys_arch_sem_wait(&mbox->mutex, 0);
    }

    if (msg != NULL) {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p msg %p\n", (void *)mbox, *msg));
        *msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
    } else {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p, null msg\n", (void *)mbox));
    }

    mbox->first++;

    if (mbox->wait_send) {
        sys_sem_signal(&mbox->not_full);
    }

    sys_sem_signal(&mbox->mutex);

    return time_needed;
}

/**
*  @brief      similar to sys_arch_mbox_fetch, however if a message is not  present in the mailbox,
               it immediately returns with the code  SYS_MBOX_EMPTY.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @return  0  OK
*          -1  ERR
*/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mb, void **msg)
{
    struct sys_mbox *mbox;
    LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
    mbox = *mb;

    sys_arch_sem_wait(&mbox->mutex, 0);

    if (mbox->first == mbox->last) {
        sys_sem_signal(&mbox->mutex);
        return SYS_MBOX_EMPTY;
    }

    if (msg != NULL) {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p msg %p\n", (void *)mbox, *msg));
        *msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
    } else {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p, null msg\n", (void *)mbox));
    }

    mbox->first++;

    if (mbox->wait_send) {
        sys_sem_signal(&mbox->not_full);
    }

    sys_sem_signal(&mbox->mutex);

    return 0;
}
#else
/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
/**
*  @brief Creates an empty mailbox for maximum "size" elements.
*  @param mb     mbox struct pointer
*  @param size
*  @return  0  OK
*          -1  ERR
*/
err_t sys_mbox_new(sys_mbox_t *mb, int size)
{
    void *msg_start;
    err_t ret = ERR_MEM;

    msg_start = (void *)malloc(size * sizeof(void *));

    if (msg_start == NULL) {
        return ERR_MEM;
    }

    int stat = aos_queue_new(mb, msg_start, size * sizeof(void *), sizeof(void *));

    if (stat == 0) {
        ret = ERR_OK;
    }

    return ret;
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief     Deallocates a mailbox. If there are messages still present in the
              mailbox when the mailbox is deallocated, it is an indication of a
              programming error in lwIP and the developer should be notified.
*  @param mb  mbox struct  pointer
*  @return    null
*/
void sys_mbox_free(sys_mbox_t *mb)
{
    void *start;

    if ((mb != NULL)) {
        start = aos_queue_buf_ptr(mb);

        if (start != NULL) {
            free(start);
        }

        aos_queue_free(mb);
    }
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief Posts the "msg" to the mailbox. This function have to block until  the "msg" is really posted.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @return  null
*/
void sys_mbox_post(sys_mbox_t *mb, void *msg)
{
    int rc, cnt = 0;

    //FIXME:
    for (;;) {
        rc = aos_queue_send(mb, &msg, sizeof(void *));
        if (rc == 0) {
            break;
        } else {
            if (cnt++ % 50 == 0)
                printf("tcp ip post send fail, queue may be full, cnt = %d, rc = %d, mb = %p\n", cnt, rc, mb);
            aos_msleep(20);
        }
    }
}

/**
*  @brief Try to post the "msg" to the mailbox. Returns ERR_MEM if this one  is full, else, ERR_OK if the "msg" is posted.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @return  0  OK
*          -1  ERR
*/
err_t sys_mbox_trypost(sys_mbox_t *mb, void *msg)
{
    if (aos_queue_send(mb, &msg, sizeof(void *)) != 0) {
        return ERR_MEM;
    } else {
        return ERR_OK;
    }
}

/*-----------------------------------------------------------------------------------*/
/*
   @brief     Blocks the thread until a message arrives in the mailbox, but does
              not block the thread longer than "timeout" milliseconds (similar to
              the sys_arch_sem_wait() function). The "msg" argument is a result
              parameter that is set by the function (i.e., by doing "*msg =
              ptr"). The "msg" parameter maybe NULL to indicate that the message
              should be dropped.

              The return values are the same as for the sys_arch_sem_wait() function:
              Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
              timeout.

              Note that a function with a similar name, sys_mbox_fetch(), is
              implemented by lwIP.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @param timeout  fetch msg in time
*  @return  time has wait
*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mb, void **msg, u32_t timeout)
{
    size_t len;
    u32_t ret;
    u32_t begin_ms, end_ms, elapsed_ms;

    if (mb == NULL) {
        return SYS_ARCH_TIMEOUT;
    }

    begin_ms = sys_now();

    if (timeout != 0UL) {

        if (aos_queue_recv(mb, timeout, msg, &len) == 0) {
            end_ms = sys_now();
            elapsed_ms = end_ms - begin_ms;
            ret = elapsed_ms;
        } else {
            ret = SYS_ARCH_TIMEOUT;
        }
    } else {
        while (aos_queue_recv(mb, AOS_WAIT_FOREVER, msg, &len) != 0);

        end_ms = sys_now();
        elapsed_ms = end_ms - begin_ms;

        if (elapsed_ms == 0UL) {
            elapsed_ms = 1UL;
        }

        ret = elapsed_ms;
    }

    return ret;
}

/**
*  @brief      similar to sys_arch_mbox_fetch, however if a message is not  present in the mailbox,
               it immediately returns with the code  SYS_MBOX_EMPTY.
*  @param mb   mbox struct pointer
*  @param msg  for mailbox to post
*  @return  0  OK
*          -1  ERR
*/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mb, void **msg)
{
    size_t len;

    if (aos_queue_recv(mb, 0u, msg, &len) != 0) {
        return SYS_MBOX_EMPTY;
    } else {
        return ERR_OK;
    }
}
#endif

/**
* @brief  Create a new mutex
* @param  mutex pointer to the mutex to create
* @return status of create new mutex
*         0  OK
*        -1  ERR
*/
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    err_t ret = ERR_MEM;
    int stat = aos_mutex_new(mutex);

    if (stat == 0) {
        ret = ERR_OK;
    }

    return ret;
}

/**
* @brief  Lock a mutex
* @param  mutex the mutex to lock
* @return null
*/
void sys_mutex_lock(sys_mutex_t *mutex)
{
    aos_mutex_lock(mutex, AOS_WAIT_FOREVER);
}

/**
* @brief  Unlock a mutex
* @param  mutex the mutex to lock
* @return null
*/
void sys_mutex_unlock(sys_mutex_t *mutex)
{
    aos_mutex_unlock(mutex);
}

/**
* @brief  Delete a mutex
* @param  mutex the mutex to lock
* @return null
**/
void sys_mutex_free(sys_mutex_t *mutex)
{
    if ((mutex != NULL)) {
        aos_mutex_free(mutex);
    }
}

/**
*  @brief  This optional function returns the current time in seconds (don't care  for wraparound,
*          this is only used for time diffs).
*  @param  null
*  @return the current time
*/
u32_t sys_now(void)
{
    return aos_now_ms();
}

/**
 * Ticks/jiffies since power up.
 */
u32_t sys_jiffies(void)
{
    return aos_now();
}

/*-----------------------------------------------------------------------------------*/
/**
*  @brief  Starts a new thread with priority "prio" that will begin its execution in the
*          function "thread()". The "arg" argument will be passed as an argument to the
*          thread() function. The id of the new thread is returned. Both the id and
*          the priority are system dependent.
*  @param  name       thread name
*  @param  thread     thread function
*  @param  arg        thread arg
*  @param  stacksize  thread stacksize
*  @param  prio       thread prio
*  @return the status of create new thread
*          0  OK
*         -1  ERR
*/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    aos_task_t task;

    int stat = aos_task_new_ext(&task, name, thread, arg, stacksize, prio);

    if (stat != 0) {
        return (sys_thread_t)0;
    }

    return task.hdl;
}

#if SYS_LIGHTWEIGHT_PROT
/*
  This optional function does a "fast" critical region protection and returns
  the previous protection level. This function is only called during very short
  critical regions. An embedded system which supports ISR-based drivers might
  want to implement this function by disabling interrupts. Task-based systems
  might want to implement this by using a mutex or disabling tasking. This
  function should support recursive calls from the same task or interrupt. In
  other words, sys_arch_protect() could be called while already protected. In
  that case the return value indicates that it is already protected.

  sys_arch_protect() is only required if your port is supporting an operating
  system.
*/
sys_prot_t sys_arch_protect(void)
{
    aos_mutex_lock(&sys_arch_mutex, AOS_WAIT_FOREVER);
    return 0;
}

/*
  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.
*/
void sys_arch_unprotect(sys_prot_t pval)
{
    LWIP_UNUSED_ARG(pval);

    aos_mutex_unlock(&sys_arch_mutex);
}
#endif

/**
*  @brief  Prints an assertion messages and aborts execution.
*  @param  null
*  @return null
*/
void sys_arch_assert(const char *f, const int l)
{
    printf("LWIP Assert %s:%d\n", f, l);
    fflush(NULL);
    abort();
}

/**
*  @brief   Is called to initialize the sys_arch layer.
*  @param   null
*  @return  null
*/
void sys_init(void)
{
    if (!aos_mutex_is_valid(&sys_arch_mutex))
        aos_mutex_new(&sys_arch_mutex);
}

#if LWIP_NETCONN_SEM_PER_THREAD
#ifndef LWIP_USER_INFO_POS
#define LWIP_USER_INFO_POS 0
#endif
#include <k_api.h>
// need RHINO_CONFIG_TASK_INFO_NUM > 3
typedef struct _sys_sem_cb {
    slist_t     next;
    void        *task_hdl;
    sys_sem_t   *sys_sem;
} sys_sem_cb_t;

/*
 * get per thread semphore
 */
sys_sem_t* sys_thread_sem_get(void)
{
    return krhino_cur_task_get()->user_info[LWIP_USER_INFO_POS];
}

sys_sem_t* sys_thread_sem_init(void)
{
    sys_sem_t *sem;
    int ret;

    sem = aos_malloc_check(sizeof(sys_sem_t));
    ret = sys_sem_new(sem, 0);
    aos_check(!ret, ERR_MEM);

    return sem;
}

void sys_thread_sem_deinit(void)
{
    // sched suspend here, because sys_sem_free will core_sched and cause mem leak
    aos_kernel_sched_suspend();
    sys_sem_t *sem = krhino_cur_task_get()->user_info[LWIP_USER_INFO_POS];

    sys_sem_free(sem);

    aos_free(sem);
    aos_kernel_sched_resume();
}

void krhino_task_create_hook_lwip_thread_sem(ktask_t *task)
{
    task->user_info[LWIP_USER_INFO_POS] = sys_thread_sem_init();
}

void krhino_task_del_hook_lwip_thread_sem(ktask_t *task, res_free_t *arg)
{
    // sched suspend here, because sys_sem_free will core_sched and cause mem leak
    aos_kernel_sched_suspend();
    sys_sem_t *sem = task->user_info[LWIP_USER_INFO_POS];

    sys_sem_free(sem);

    aos_free(sem);
    aos_kernel_sched_resume();
}
#endif
