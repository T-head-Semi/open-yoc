/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include "mqueue.h"

mqd_t mq_open(const char *name, int oflag, ...)
{
    aos_status_t status;
    aos_queue_t *mq = aos_zalloc(sizeof(aos_queue_t));

    if (mq == NULL) {
        return NULL;
    }

    status = aos_queue_create(mq, DEFAULT_MQUEUE_SIZE, DEFAULT_MAX_MSG_SIZE, 0);
    if (status != 0) {
        return NULL;
    }

    return mq;
}

int mq_close(mqd_t mqdes)
{
    if (mqdes) {
       aos_queue_free(mqdes);
       aos_free(mqdes);
       mqdes = NULL;
       return 0;
    }
    return -1;
}

ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio)
{
    int ret;
    size_t msg_size;

    *msg_prio = 0;
    ret = aos_queue_recv(mqdes, 0, msg_ptr, &msg_size);
    if (ret != 0) {
        return 0;
    }

    return msg_size;
}

int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio)
{
    int ret;

    ret = aos_queue_send(mqdes, (void *)msg_ptr, msg_len);
    if (ret != 0) {
        return 0;
    }

    return msg_len;
}

int mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat, struct mq_attr *omqstat)
{
    return 0;
}

int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat)
{
    return 0;
}

ssize_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                        unsigned *msg_prio, const struct timespec *abs_timeout)
{
    int ret;
    size_t msg_size;
    int timeout_ms;

    *msg_prio = 0;

    timeout_ms = abs_timeout->tv_sec * 1000 + abs_timeout->tv_nsec / 1000;
    ret = aos_queue_recv(mqdes, timeout_ms, msg_ptr, &msg_size);
    if (ret != 0) {
        return 0;
    }

    return msg_size;
}

int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
                 unsigned msg_prio, const struct timespec *abs_timeout)
{
    int ret;

    (void)msg_prio;

    ret = aos_queue_send(mqdes, (void *)msg_ptr, msg_len);
    if (ret != 0) {
        return 0;
    }

    return msg_len;
}
