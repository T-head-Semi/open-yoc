/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <stdint.h>
#include <aos/kernel.h>
#include "vfs_adapt.h"

void *vfs_lock_create(void)
{
    int32_t   ret;
    aos_mutex_t *mutex;

    mutex = aos_malloc(sizeof(aos_mutex_t));
    if (mutex == NULL) {
        return NULL;
    }
    ret = aos_mutex_new(mutex);
    if (ret != 0) {
        aos_free(mutex);
        return NULL;
    }

    return (void *)mutex;
}

int32_t vfs_lock_free(void *lock)
{
    aos_mutex_t *m = (aos_mutex_t *)lock;

    if (m == NULL) {
        return -1;
    }
    aos_mutex_free(m);
    aos_free(m);

    return 0;
}

int32_t vfs_lock(void *lock)
{
    int ret = aos_mutex_lock((aos_mutex_t *)lock, AOS_WAIT_FOREVER);
    return ret;
}

int32_t vfs_unlock(void *lock)
{
    int ret =  aos_mutex_unlock((aos_mutex_t *)lock);
    return ret;
}

void *vfs_malloc(uint32_t size)
{
    return aos_malloc(size);
}

void vfs_free(void *ptr)
{
    aos_free(ptr);
}
