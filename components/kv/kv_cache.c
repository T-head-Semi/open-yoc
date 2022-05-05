/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#if (CONFIG_KV_ENABLE_CACHE || CONFIG_KV_START_OPT)
#include "kv_cache.h"

/**
 * @brief  malloc & init cache node for kv
 * @param  [in] kv
 * @param  [in] num count of cache-nodes
 * @return
 */
void kv_cache_nodes_init(kv_t *kv, size_t num)
{
    size_t size;

    size        = sizeof(cache_node_t) * num;
    kv->node_nb = num;
    kv->nodes   = malloc(size);
    memset(kv->nodes, 0xff, size);
}

/**
 * @brief  reset the cache node
 * @param  [in] node
 * @return
 */
void kv_cache_node_reset(cache_node_t *node)
{
    node->block_id = CACHE_INVALID_VAL;
    node->offset   = CACHE_INVALID_VAL;
}

/**
 * @brief  request one cache node index unused
 * @param  [in] kv
 * @return -1 on error
 */
int kv_cache_node_get(kv_t *kv)
{
    int i;
    int inc = 32;

    for (i = 0; i < kv->node_nb; i++) {
        if (kv->nodes[i].block_id == CACHE_INVALID_VAL) {
            return i;
        }
    }

    kv->nodes = realloc(kv->nodes, (kv->node_nb + inc) * sizeof(cache_node_t));
    if (!kv->nodes) {
        printf("error happens, cache node get may be oom, node num = %lu!\n", kv->node_nb);
        return -1;
    }

    memset(&kv->nodes[kv->node_nb], 0xff, inc * sizeof(cache_node_t));
    kv->node_nb += inc;
    return i;
}

/**
 * @brief  put the key with other params to the inner-hash_map of the kv
 * @param  [in] kv
 * @param  [in] key
 * @param  [in] block_id
 * @param  [in] offset : head_offset of the key in one block
 * @return
 */
void kv_cache_node_in(kv_t *kv, const char *key, int block_id, uint32_t offset)
{
    long idx;
    int valid = 0;
    cache_node_t *cache;

    idx = (long)hash_get2(&kv->map, key, &valid);
    if (valid) {
        cache           = &kv->nodes[idx];
        cache->block_id = block_id;
        cache->offset   = offset;
    } else {
        idx = kv_cache_node_get(kv);
        if (idx >= 0) {
            cache           = &kv->nodes[idx];
            cache->block_id = block_id;
            cache->offset   = offset;
            hash_set(&kv->map, key, (void*)idx);
        } else {
            printf("error: cache in may be oom\n");
            return;
        }
    }
}

/**
 * @brief  remove the key from the inner-hash_map of the kv
 * @param  [in] kv
 * @param  [in] key
 * @return
 */
void kv_cache_node_out(kv_t *kv, const char *key)
{
    long idx;
    int rc, valid = 0;
    cache_node_t *cache;

    idx = (long)hash_get2(&kv->map, key, &valid);
    rc  = hash_del(&kv->map, key);
    if (valid && rc == 0) {
        cache = &kv->nodes[idx];
        kv_cache_node_reset(cache);
    } else {
        printf("error happens in kv cache out, valid = %d, rc = %d, key = %s\n", valid, rc, key);
    }

    return;
}

#endif


