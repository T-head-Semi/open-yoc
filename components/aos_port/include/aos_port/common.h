/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#ifndef __AOS_PORT_COMMON_H__
#define __AOS_PORT_COMMON_H__

#include "aos_port/port.h"

#ifdef __linux__
#include "aos_port/list.h"

#define aos_malloc         malloc
#define aos_realloc        realloc
#define aos_calloc         calloc
#define aos_zalloc(size)   calloc(1, size)
#define aos_free           free
#define aos_freep          freep

#define AOS_WAIT_FOREVER                          (-1)
#define aos_mutex_t                               pthread_mutex_t
#define aos_mutex_new(m)                          pthread_mutex_init(m, NULL)
#define aos_mutex_lock(m,t)                       pthread_mutex_lock(m)
#define aos_mutex_unlock(m)                       pthread_mutex_unlock(m)
#define aos_mutex_free(m)                         pthread_mutex_destroy(m)

#define aos_msleep(s)                             usleep((s) * 1000)
#define aos_now_ms()                              get_now_ms()
#define aos_assert(x)                             assert(x)

#define aos_sem_t                                 sem_t
#define aos_sem_new(m,t)                          sem_init(m, 0, t)
#define aos_sem_signal(m)                         sem_post(m)
#define aos_sem_wait(m, t)                        sem_wait1(m, t)
#define aos_sem_free(m)                           sem_destroy(m)

#define aos_task_t                                pthread_t
#define AOS_DEFAULT_APP_PRI                       (32)
#define aos_task_new(name, stub, arg, stack_size, priority) pthread_spawn(NULL, (void*)stub, arg, stack_size, AOS_DEFAULT_APP_PRI, name)
#define aos_task_new_ext(pid, name, stub, arg, stack_size, priority) pthread_spawn(pid, (void*)stub, arg, stack_size, priority, name)
#define aos_task_exit(x)

#define aos_open       open
#define aos_close      close
#define aos_read       read
#define aos_stat       stat
#define aos_lseek      lseek

#endif

#if !defined(__BEGIN_DECLS__) || !defined(__END_DECLS__)
#if defined(__cplusplus)
#  define __BEGIN_DECLS__ extern "C" {
#  define __END_DECLS__   }
#else
#  define __BEGIN_DECLS__
#  define __END_DECLS__
#endif
#endif

#ifndef CHECK_PARAM
#define CHECK_PARAM(x, ret) \
	do { \
		if (!(x)) { \
			return ret; \
		}\
	} while (0)
#endif

#ifndef CHECK_RET_WITH_GOTO
#define CHECK_RET_WITH_GOTO(x, label) \
	do { \
		if (!(x)) { \
			printf("%s, %d fail.\n", __FUNCTION__, __LINE__); \
			goto label; \
		}\
	} while (0)
#endif

#ifndef CHECK_RET_WITH_RET
#define CHECK_RET_WITH_RET(x, ret) \
	do { \
		if (!(x)) { \
			printf("%s, %d fail.\n", __FUNCTION__, __LINE__); \
			return ret; \
		}\
	} while (0)
#endif

#ifndef CHECK_RET_TAG_WITH_GOTO
#define CHECK_RET_TAG_WITH_GOTO(x, label) \
	do { \
		if (!(x)) { \
			LOGE(TAG, "%s, %d fail", __FUNCTION__, __LINE__); \
			goto label; \
		}\
	} while (0)
#endif

#ifndef CHECK_RET_TAG_WITH_RET
#define CHECK_RET_TAG_WITH_RET(x, ret) \
	do { \
		if (!(x)) { \
			LOGE(TAG, "%s, %d fail", __FUNCTION__, __LINE__); \
			return ret; \
		}\
	} while (0)
#endif

#if (!defined(aos_check_return_val))
#define aos_check_return_val(X, ret)                                                               \
    do {                                                                                           \
        if (!(X)) {                                                                      \
			LOGE(TAG, "%s, %d fail", __FUNCTION__, __LINE__);                                      \
            return ret;                                                                            \
        }                                                                                          \
    } while (0)
#endif

#if (!defined(aos_check_return_einval))
#define aos_check_return_einval(X) aos_check_return_val(X, -EINVAL)
#endif

#if (!defined(aos_check_return_null))
#define aos_check_return_null(X) aos_check_return_val(X, NULL)
#endif

#if (!defined(aos_check_return_enomem))
#define aos_check_return_enomem(X) aos_check_return_val(X, -ENOMEM)
#endif

#if (!defined(aos_check_return))
#define aos_check_return(X)                                                                        \
    do {                                                                                           \
        if (unlikely(!(X))) {                                                                      \
            except_process(errno);                                                                 \
            return;                                                                                \
        }                                                                                          \
    } while (0)
#endif

#endif /* __AOS_PORT_COMMON_H__ */

