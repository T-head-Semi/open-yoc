/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#ifndef __AOS_PORT_H__
#define __AOS_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/time.h>
#include <semaphore.h>
#include <mqueue.h>
#include <pthread.h>

int sem_wait1(sem_t *sem, unsigned int timeout);

long long get_now_ms();

void freep(char **ptr);

int pthread_spawn(pthread_t *thread, void *(*start_routine) (void *), void *arg, size_t stack_size, size_t priority,
                  const char *name);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __AOS_PORT_H__ */

