/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#if (CONFIG_KV_ENABLE_CACHE || CONFIG_KV_START_OPT)
#ifndef __KV_CACHE_TYPEDEF_H__
#define __KV_CACHE_TYPEDEF_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <aos/hash.h>
#include <aos/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CACHE_INVALID_VAL (0xFFFF)

typedef struct cache_node {
    uint16_t           block_id;
    uint16_t           offset;
} cache_node_t;

//FIXME: kv-conflict may be one only, list no-needed
typedef struct cache_node_list {
    char               *key;
    struct cache_node  cache;
    slist_t            node;
} lcache_node_t;

#ifdef __cplusplus
}
#endif

#endif /* __KV_CACHE_TYPEDEF_H__ */
#endif

