/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <aos/list.h>

#include "voice.h"

typedef struct {
    slist_t next;
    void *data_bake;
    void *data_cur;
} voice_heap_t;

slist_t g_voice_heap_head;

void *voice_malloc(unsigned int size)
{
    voice_heap_t *node = aos_malloc(sizeof(voice_heap_t));

    node->data_bake = aos_malloc_check(size + VOICE_DCACHE_OFFSET*2);

    if (((uint32_t)node->data_bake & 0xfffffff0) != 0) {
        node->data_cur = (char *)(((int)node->data_bake + VOICE_DCACHE_OFFSET) & 0xfffffff0);
    } else {
        node->data_cur = node->data_bake;
    }

    slist_add(&node->next, &g_voice_heap_head);

    return node->data_cur;
}

void voice_free(void *data)
{
    voice_heap_t *node;
    slist_t *tmp;

    slist_for_each_entry_safe(&g_voice_heap_head, tmp, node, voice_heap_t, next) {
        if (data == node->data_cur) {
            aos_free(node->data_bake);
            slist_del(&node->next, &g_voice_heap_head);
            break;
        }
    }
}

void voice_rb_create(voice_ringbuf_t *rb, size_t size)
{
    rb->pbuf = aos_zalloc_check(size * sizeof(voice_data_t *));
    rb->length = size;
    rb->ridx = 0;
    rb->widx = 0;
}

voice_data_t *voice_rb_get(voice_ringbuf_t *rb)
{
    voice_data_t *ele = NULL;

    if (rb->widx != rb->ridx) {
        ele = rb->pbuf[rb->ridx];
        rb->ridx = (rb->ridx + 1) % rb->length;
    }

    return ele;
}

int voice_rb_put(voice_ringbuf_t *rb, const char *data, size_t len)
{
    aos_check_param(data && len > 0);

    /* check if ringbuf is full */
    if ((rb->widx + 1) % rb->length != rb->ridx)
    {
        voice_data_t *ele;
        ele = rb->pbuf[rb->widx];
        if (ele == NULL || ele->len != len) {
            aos_free(ele);
            ele = aos_malloc_check(sizeof(voice_data_t) + len);
            ele->len = len;
        }
        rb->pbuf[rb->widx] = ele;

        memcpy(rb->pbuf[rb->widx]->data, data, len);
        rb->widx = (rb->widx + 1) % rb->length;
        return 0;
    } else {
        LOGW("rb", "voice rb is full");
    }

    return -1;
}

void voice_rb_destroy(voice_ringbuf_t *rb)
{
    aos_free(rb->pbuf);
    rb->length = 0;
    rb->ridx = 0;
    rb->widx = 0;
}
