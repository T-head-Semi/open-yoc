/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "kvset.h"
#include "block.h"

static void kv_verify(kv_t *kv);
#if CONFIG_KV_ENABLE_CACHE

static void _cache_nodes_init(kv_t *kv, size_t num)
{
    size_t size;

    size        = sizeof(cache_node_t) * num;
    kv->node_nb = num;
    kv->nodes   = malloc(size);
    memset(kv->nodes, 0xff, size);
}

static void _cache_node_reset(cache_node_t *node)
{
    node->block_id = CACHE_INVALID_VAL;
    node->offset   = CACHE_INVALID_VAL;
}

static int _cache_node_get(kv_t *kv)
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
        printf("error happens, cache node get may be oom, node num = %u!\n", kv->node_nb);
        return -1;
    }

    memset(&kv->nodes[kv->node_nb], 0xff, inc * sizeof(cache_node_t));
    kv->node_nb += inc;
    return i;
}

static void _kv_cache_in(kv_t *kv, const char *key, int block_id, uint32_t offset)
{
    int idx, valid = 0;
    cache_node_t *cache;

    idx = (int)hash_get2(&kv->map, key, &valid);
    if (valid) {
        cache           = &kv->nodes[idx];
        cache->block_id = block_id;
        cache->offset   = offset;
    } else {
        idx = _cache_node_get(kv);
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

static void _kv_cache_out(kv_t *kv, const char *key)
{
    int rc, idx, valid = 0;
    cache_node_t *cache;

    idx = (int)hash_get2(&kv->map, key, &valid);
    rc  = hash_del(&kv->map, key);
    if (valid && rc == 0) {
        cache = &kv->nodes[idx];
        _cache_node_reset(cache);
    } else {
        printf("error happens in kv cache out, valid = %d, rc = %d, key = %s\n", valid, rc, key);
    }

    return;
}

static int _iter_cache_node(kvnode_t *node, void *p)
{
    int idx;
    cache_node_t *cache = NULL;
    kv_t *kv            = (kv_t *)p;

    if (NODE_VAILD(node)) {
        idx = _cache_node_get(kv);
        if (idx >= 0) {
            cache           = &kv->nodes[idx];
            cache->block_id = node->block->id;
            cache->offset   = node->head_offset;
            hash_set(&kv->map, (const char *)KVNODE_OFFSET2CACHE(node, head_offset), (void*)idx);
        } else {
            printf("error: iter cache node may be oom\n");
            return -1;
        }
    }
    return 0;
}
#endif

/**
 * @brief  init the kv fs
 * @param  [in] kv
 * @param  [in] mem        : the start addrress of flash or mem, etc
 * @param  [in] block_num  : number of blocks
 * @param  [in] block_size : size of per-block
 * @return 0/-1
 */
int kv_init(kv_t *kv, uint8_t *mem, int block_num, int block_size)
{
    int i, gc_size = 0;
    kv->num    = block_num;
    kv->mem    = mem;
    kv->blocks = calloc(kv->num, sizeof(kvblock_t));
    if (kv->blocks == NULL) {
        return -1;
    }
    kv->bid    = 0;
    kv->gc_bid = -1;

    for (i = 0; i < block_num; i++) {
        kv->blocks[i].kv = kv;
        kv->blocks[i].id = i;
        kvblock_init(kv->blocks + i, mem + i * block_size, block_size);

        int v = kvblock_free_size(kv->blocks + i);
        if (v >= gc_size && v > 4) {
            gc_size = v;
            kv->gc_bid = i;
        } else if (kv->bid == 0)
            kv->bid = i;
    }

    kv_verify(kv);

#if CONFIG_KV_ENABLE_CACHE
    hash_init(&kv->map, 32);
    _cache_nodes_init(kv, 64);
    kv_iter(kv, _iter_cache_node, kv);
#endif
    return 0;
}

static void kv_verify(kv_t *kv)
{
    for (int i = 0; i < kv->num; i++) {
        kvnode_t node_1;

        /* load block 1 */
        kvblock_t *block_1 = &kv->blocks[i];
        kvblock_cache_malloc(block_1);

        uint8_t *next = block_1->mem_cache;
        while (kvblock_search(block_1, next, &node_1) == 0) {
            if (NODE_VAILD(&node_1)) {
                for (int j = i; j < kv->num; j++) {
                    kvnode_t node_2;

                    /* load block 2 */
                    kvblock_t *block_2 = &kv->blocks[j];
                    kvblock_cache_malloc(block_2);

                    uint8_t *next = j == i ? KVNODE_OFFSET2CACHE(&node_1, next_offset) :block_2->mem_cache;
                    while (kvblock_search(block_2, next, &node_2) == 0) {
                        if (NODE_VAILD(&node_2)) {
                            if (kvblock_check_version(&node_1, &node_2) == &node_1) {
                                /* free block 2 */
                                kvblock_cache_free(block_2);
                                goto out;
                            }
                        }
                        next = KVNODE_OFFSET2CACHE(&node_2, next_offset);
                    } /* while block2 */

                    /* free block 2 */
                    kvblock_cache_free(block_2);
                }
            }
out:
            next = KVNODE_OFFSET2CACHE(&node_1, next_offset);
        }/* while block1 */

        /* free block 1 */
        kvblock_cache_free(block_1);
    }
}

/**
 * @brief  iterate all valid kv pair
 * @param  [in] kv
 * @param  [in] fn   : callback
 * @param  [in] data : opaque of the fn callback
 * @return 0 on success
 */
int kv_iter(kv_t *kv, int (*fn)(kvnode_t *, void *), void *data)
{
    int ret = -1;
    for (int i = 0; i < kv->num; i++) {
        ret = kvblock_iter(kv->blocks + i, NODE_EXISTS, fn, data);
        if (ret != 0)
            break;
    }

    return ret;
}

/**
 * @brief  find the kvnode by key
 * @param  [in] kv
 * @param  [in] key
 * @param  [in] node : used for store the result finding
 * @return 0 if find
 */
int kv_find(kv_t *kv, const char *key, kvnode_t *node)
{
    int found = -1;

#if CONFIG_KV_ENABLE_CACHE
    int idx, valid = 0;
    cache_node_t *cache;

    idx = (int)hash_get2(&kv->map, key, &valid);
    if (valid) {
        cache = &kv->nodes[idx];
        kvblock_t *block = &kv->blocks[cache->block_id];
        kvblock_cache_malloc(block);

        uint8_t *next = block->mem_cache + cache->offset;
        kvblock_search(block, next, node);
        int cmp_res = kvnode_cmp_name(node, key);
        kvblock_cache_free(block);
        if (cmp_res == 0)
            return 0;
    }
#else
    for (int i = 0; i < kv->num; i++) {
        if (kvblock_find(kv->blocks + i, key, node) == 0) {
            found = 0;
            if (node->rw != 0)
                return 0;
        }
    }
#endif

    return found;
}

static int _kvblock_deep_gc(kvnode_t *node, void *data)
{
    kvblock_t *block = (kvblock_t *)data;

    if (node->rw == 1) {
        int offset;
        int version = node->version == 255 ? 1 : node->version + 1;
        const char *key = (const char *)KVNODE_OFFSET2CACHE(node, head_offset);

        offset = kvblock_set(block, key, KVNODE_OFFSET2CACHE(node, value_offset), node->val_size, version);
        if (offset >= 0) {
#if CONFIG_KV_ENABLE_CACHE
            int idx, valid = 0;
            cache_node_t *cache;
            kv_t *kv = block->kv;

            idx = (int)hash_get2(&kv->map, key, &valid);
            if (valid) {
                cache           = &kv->nodes[idx];
                cache->block_id = block->id;
                cache->offset   = offset;
            } else {
                printf("deep gc may be error\n");
            }
#endif
            kvnode_rm(node);
        }
    }

    return 0;
}

// 将从最小使用的块开始，将所有可以放到整理块的块，都放到整理块中
int kv_gc(kv_t *kv)
{
    int new_gc_id = -1;

    while (1) {
        int min_id   = -1;
        int min_size = kv->blocks[kv->gc_bid].size - kv->blocks[kv->gc_bid].write_offset;

        // get min used block
        for (int i = 0; i < kv->num; i++) {
            if (kv->blocks[i].ro_count == 0 && i != kv->gc_bid && \
                kv->blocks[i].kv_size > 0 && kv->blocks[i].kv_size < min_size) {
                min_size = kv->blocks[i].kv_size;
                min_id   = i;
            }
        }

        if (min_id != -1) {
            kvblock_iter(kv->blocks + min_id, 1, _kvblock_deep_gc, kv->blocks + kv->gc_bid);
            if (kv->blocks[min_id].count == 0)
                new_gc_id = min_id;
        } else {
            break;
        }
    }

    if (new_gc_id != -1) {
        kv->gc_bid = new_gc_id;

        return 0;
    }

    return -1;
}

/**
 * @brief  set key-value pair
 * @param  [in] kv
 * @param  [in] key
 * @param  [in] value
 * @param  [in] size  : size of the value
 * @return size on success
 */
int kv_set(kv_t *kv, const char *key, void *value, int size)
{
    if (key == NULL || value == NULL || size <= 0 || strlen(key) + size >= 498)
        return -1;

    kvnode_t node;
    int      kv_exist;
    int      version  = 0;
    int      gc_count = 0;
    int      start_bid;

start1:
    kv_exist = kv_find(kv, key, &node) == 0;
    /* kvnode rm no mem opt, ignore call kvblock_cache_malloc */

    if (kv_exist) {
        kvblock_cache_malloc(node.block);
        if (node.val_size == (uint16_t)size && \
            memcmp(((void *)KVNODE_OFFSET2CACHE(&node, value_offset)), value, size) == 0) {
            kvblock_cache_free(node.block);
            return size;
        }
        version = node.version;
        kvblock_cache_free(node.block);
        if (version == 255)
            version = 0;
    }
    version++;

start2:
    start_bid = kv->bid;
    while (1) {
        if (kv->blocks[kv->bid].ro_count == 0 && kv->bid != kv->gc_bid) {
            int offset = kvblock_set(kv->blocks + kv->bid, key, value, size, version);
            if (offset >= 0) {
                if (kv_exist) {
                    kvnode_rm(&node);
#if CONFIG_KV_ENABLE_CACHE
                    _kv_cache_out(kv, key);
                }
                _kv_cache_in(kv, key, kv->bid, offset);
#else
                }
#endif
                return size;
            }
        }

        kv->bid = (kv->bid + 1) % kv->num;
        if (kv->bid == start_bid)
            break;
    }

    if (gc_count == 0 && kv_gc(kv) == 0)
    {
        gc_count = 1;
        if (kv_exist)
            goto start1;
        else
            goto start2;
    }

    return -1;
}

/**
 * @brief  get value by the key-string
 * @param  [in] kv
 * @param  [in] key
 * @param  [in] value
 * @param  [in] size  : size of the value
 * @return > 0 on success
 */
int kv_get(kv_t *kv, const char *key, void *value, int size)
{
    kvnode_t node;

    if (kv_find(kv, key, &node) == 0) {

        /* kv find result node, block mem_cache maybe NULL*/
        kvblock_cache_malloc(node.block);

        uint8_t* node_value = KVNODE_OFFSET2CACHE(&node, value_offset);
        if (node.rw) {
            //rw normal node
            memcpy(value, node_value, node.val_size < size ? node.val_size : size);

            kvblock_cache_free(node.block);
            return node.val_size;
        } else {
            //ro node, string mode, key1="123", key2=123
            if (node.val_size > 0) {

                int ret = node.val_size;
                if (node_value[0] == '\"') {
                    //string value
                    if (node_value[ret - 1] == '\r') {
                        ret --;
                    }

                    if (node_value[ret - 1] == '\"') {
                        ret --;
                    }

                    ret --; /* del first " */
                    ret = ret < size ? ret : size;
                    memcpy(value, node_value + 1, ret);

                    kvblock_cache_free(node.block);
                    return ret;

                } else {
                    //number
                    int num = atoi((char *)node_value);
                    memcpy(value, &num, 4 < size ? 4 : size);

                    kvblock_cache_free(node.block);
                    return 4;
                }
            } /* val_size > 0 */
        }/* else rw */
    }/* kv find */
    return -1;
}

/**
 * @brief  delete the key from kv fs
 * @param  [in] kv
 * @param  [in] key
 * @return 0 on success
 */
int kv_rm(kv_t *kv, const char *key)
{
    kvnode_t node;
    int ret = kv_find(kv, key, &node);

    /* kvnode rm no mem opt, ignore call kvblock_cache_malloc */
    if (ret == 0) {
        kvnode_rm(&node);
#if CONFIG_KV_ENABLE_CACHE
        _kv_cache_out(kv, key);
#endif
    }

    return ret;
}

/**
 * @brief  reset the kv fs
 * @param  [in] kv
 * @return 0/-1
 */
int kv_reset(kv_t *kv)
{
    for (int i = 0; i < kv->num; i++)
        kvblock_reset(kv->blocks + i);
#if CONFIG_KV_ENABLE_CACHE
    memset(kv->nodes, 0xff, kv->node_nb * sizeof(cache_node_t));
#endif

    return 0;
}

void kv_dump(kv_t *kv)
{
    printf("bid=%d, gc_id=%d\n", kv->bid, kv->gc_bid);
    for (int i = 0; i < kv->num; i++) {
        printf("block id: %d, diry_size = %d, kv_size = %d, count = %d\n", i,
               kv->blocks[i].dirty_size, kv->blocks[i].kv_size, kv->blocks[i].count);
        kvblock_dump(kv->blocks + i);
    }
}

void kv_show_data(kv_t *kv)
{
    for (int i = 0; i < kv->num; i++) {
        kvblock_show_data(kv->blocks + i, 8);

        printf("\n");
    }
}
