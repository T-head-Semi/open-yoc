/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#include "aos_port/port.h"

#ifdef __linux__

int sem_wait1(sem_t *sem, unsigned int timeout)
{
    if (timeout == -1) {
        return sem_wait(sem);
    } else {
        uint64_t nsec;
        struct timespec ts;
        struct timeval now;

        gettimeofday(&now, NULL);
        nsec       = now.tv_usec * 1000 + (timeout % 1000) * 1000000;
        ts.tv_nsec = nsec % 1000000000;
        ts.tv_sec  = now.tv_sec + nsec / 1000000000 + timeout / 1000;

        return sem_timedwait(sem, &ts);
    }
}

long long get_now_ms()
{
    long long ms;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    ms = tv.tv_sec*1000 + tv.tv_usec/1000;

    return ms;
}

void freep(char **ptr)
{
    if (ptr && (*ptr)) {
        free(*ptr);
        *ptr = NULL;
    }
}

int pthread_spawn(pthread_t *thread, void *(*start_routine) (void *), void *arg, size_t stack_size, size_t priority,
                  const char *name)
{
    int rc;
    pthread_attr_t attr;

    (void)name;
    (void)priority;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size + 4*1024);
    rc = pthread_create(thread, &attr, start_routine, arg);
    pthread_attr_destroy(&attr);

    return rc;
}

#endif
