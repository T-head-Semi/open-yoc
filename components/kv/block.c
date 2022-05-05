/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#ifndef linux
#include <aos/debug.h>
#endif

#include "kv_cache.h"
#include "kvset.h"
#include "block.h"

#define RW_FLAG_SIZE 5

static inline int align4(int x)
{
    return (x % 4 != 0) ? (4 - x % 4 + x) : x;
}

static int keycmp(const uint8_t *s, uint8_t *d, int n)
{
    if (n == 0)
        return 0;

    while (*s != 0 && *s == *d) {
        s++;
        d++;
        n--;
        if (n < 0)
            return -1;
    }
    if (n == 0 && *s == 0)
        return 0;

    return *s - *d;
}

void kvnode_show(kvnode_t *node)
{
    printf("  [%d]: size:%3d/%3d, erase:%08x, version:%3d, key:%.*s, values:%.*s, next:%d\n",
           node->block->id,
           node->val_size, node->node_size,
           node->erase_flag,
           node->version,
           (int)(node->value_offset - node->head_offset - 1), KVNODE_OFFSET2CACHE(node, head_offset),
           node->val_size, KVNODE_OFFSET2CACHE(node, value_offset),
           node->next_offset
          );
}

static void block_erase(kvblock_t *block)
{
    int rc = block->kv->ops->erase(block->kv, (long)block->mem - (long)block->kv->mem, block->size);
    if (rc < 0) {
        printf("kv: erase failed, rc = %d. id = %d\n", rc, block->id);
    }
}

static int block_write(kvblock_t *block, int offset, void *data, int size)
{
    int rc = block->kv->ops->write(block->kv, (long)block->mem + offset - (long)block->kv->mem, data, size);
    if (rc < 0) {
        printf("kv: write failed, rc = %d. id = %d, offset = %d, size = %d\n", rc, block->id, offset, size);
    }

    return rc;
}

int block_read(kvblock_t *block, int offset, void *data, int size)
{
    int rc = block->kv->ops->read(block->kv, (long)block->mem + offset - (long)block->kv->mem, data, size);
    if (rc < 0) {
        printf("kv: read failed, rc = %d. id = %d, offset = %d, size = %d\n", rc, block->id, offset, size);
    }

    return rc;
}

static void __kvblock_gc(kvblock_t *block)
{
    block_erase(block);

    block->write_offset = 0;
    block->kv_size      = 0;
    block->dirty_size   = 0;
    block->count        = 0;
}

/**
 * @brief  init the block
 * @param  [in] block
 * @param  [in] mem  : addr of the block
 * @param  [in] size : size of block mem
 * @return
 */
void kvblock_init(kvblock_t *block, uint8_t *mem, int size)
{
    block->size = size;
    block->mem  = mem;

    kvblock_calc(block);

    if (block->count == 0) {
#ifdef CONFIG_NON_ADDRESS_FLASH
        uint32_t *v = (uint32_t*)malloc(block->size);
        block_read(block, 0, v, block->size);
#else
        uint32_t *v = (uint32_t *)block->mem;
#endif
        int gc = 0;
        for (int i = 1; i < block->size / 4; i++) {
            if (v[i] != v[0]) {
                __kvblock_gc(block);
                gc = 1;
                break;
            }
        }

        if (gc == 0 && v[0] == 0) {
            __kvblock_gc(block);
        }

#ifdef CONFIG_NON_ADDRESS_FLASH
        free(v);
#endif
    }
}

void kvnode_rm(kvnode_t *node)
{
    // printf("rm node version = %d\n", node->version);
    if (NODE_VAILD(node) && node->rw != 0) {
        kvblock_t *block = node->block;
        if (block->count - block->ro_count > 1) {
            int x = ERASE_FLAG;

            block_write(block, node->next_offset - 4, &x, 4);
            block->dirty_size += node->node_size;
            block->kv_size -= node->node_size;

            block->count--;
        } else {
            __kvblock_gc(block);
        }
    }
}

/**
 * @brief  alloc a kv node, return node start write address
 * @param  [in] block
 * @param  [in] size
 * @return -1 on error
 */
int kvblock_alloc_node(kvblock_t *block, int size)
{
    if (block->write_offset + size <= block->size - 4) {
        int offset = block->write_offset;
        block->write_offset += size;

        return offset;
    }

    return -1;
}

/**
 * @brief  write the kv pair to the block
 * @param  [in] block
 * @param  [in] key
 * @param  [in] value
 * @param  [in] size    : size of the value
 * @param  [in] version : 1~255
 * @return -1 on error
 */
int kvblock_set(kvblock_t *block, const char *key, void *value, int size, int version)
{
    int len = strlen(key);
    if (len <= 0 || len > 0x3F || size > 0x3FFFF) // key length : >= 1 and <= 63, value <= 0x3FFFF
        return -1;

    int malloc_size = align4(len + size + RW_FLAG_SIZE + 1);
    int node_size   = malloc_size + 4;
    int offset      = kvblock_alloc_node(block, node_size);

    if (offset >= 0) {
        uint8_t *buffer = calloc(1, malloc_size);
        uint8_t size_lo = (size  >> 0) & 0xFF;
        uint8_t size_hi = ((size  >> 8) & 0xFF) | (len << 2);

        // write key
        strcpy((char *)buffer, key);

        buffer[len++] = 0;
        buffer[len++] = size_lo;
        buffer[len++] = size_hi;
        buffer[len++] = version & 0xFF;
        buffer[len++] = '=';

        // write value
        memcpy(buffer + len, value, size);
        len += size;

        buffer[len++] = size_hi;

        block_write(block, offset, buffer, malloc_size);

#ifdef CONFIG_NON_ADDRESS_FLASH
        uint8_t *buffer_verify = calloc(1, malloc_size);
        block_read(block, offset, buffer_verify, malloc_size);
#else
        uint8_t *buffer_verify = block->mem + offset;
#endif
        if (memcmp(buffer_verify, buffer, malloc_size) == 0) {
            block->kv_size += node_size;
            block->count++;
        } else {
            printf("kv: write verify failed, may be have bad block. id = %d, offset = %d\n", block->id, offset);
            int zero = ERASE_FLAG;
            block_write(block, offset + malloc_size, &zero, 4);
            offset = -1;
        }

        free(buffer);
#ifdef CONFIG_NON_ADDRESS_FLASH
        free(buffer_verify);
#endif
    }

    return offset;
}

/**
 * @brief  search kv-node in the block
 * @param  [in] block
 * @param  [in] c    : start addr begin search of the block
 * @param  [in] node
 * @return 0/-1
 */
int kvblock_search(kvblock_t *block, uint8_t *c, kvnode_t *node)
{
    uint8_t *delim = NULL;

    // found first char
    while (c - block->mem_cache < block->size && *c == 0)
        c++;

    node->block = block;
    node->head_offset = KVNODE_CACHE2OFFSET(node, c);

    while (c - block->mem_cache < block->size - 1) {
        switch (*c) {
        case '\0': {
            uint16_t value_size = c[1] | ((c[2] << 8) & 0x3FF);
            uint16_t key_size   = c[2] >> 2;

            if (c[4] != '=' || c + value_size - block->mem_cache >= block->size ||
                c - key_size < block->mem_cache || c[2] != c[value_size + RW_FLAG_SIZE])
                break;

            node->node_size = align4(key_size + value_size + RW_FLAG_SIZE + 1) + 4;
            node->version = c[3];
            node->val_size  = value_size;
            node->erase_flag = *(int *)(c - key_size + node->node_size - 4);
            node->rw         = 1;

            node->head_offset = KVNODE_CACHE2OFFSET(node, c) - key_size;
            node->value_offset = KVNODE_CACHE2OFFSET(node, c) + RW_FLAG_SIZE;
            node->next_offset = node->head_offset + node->node_size;

#ifndef linux
            aos_assert(node->node_size < block->size);
#endif
            return 0;
        }

        case '=':
            delim = c;
            break;

        case '\n':
            if (delim) {
                node->version    = 0;
                node->val_size   = c - delim - 1;
                node->erase_flag = 0xFFFF;
                node->rw         = 0;

                node->value_offset =  KVNODE_CACHE2OFFSET(node, delim) + 1;
                node->next_offset = KVNODE_CACHE2OFFSET(node, c) + 1;
                node->node_size = node->next_offset - node->head_offset;

#ifndef linux
                aos_assert(node->node_size < block->size);
#endif
                return 0;
            }
            break;

        case '\r':
            break;

        default:
            if (delim && !isprint(*c))
                delim = NULL;
            break;
        }

        c++;
    }

    return -1;
}

/**
 * @brief  check the two kvnode(same key) & delete the old one
 * @param  [in] node1
 * @param  [in] node2
 * @return node of the delete
 */
kvnode_t *kvblock_check_version(kvnode_t *node1, kvnode_t *node2)
{
    uint8_t *head1 = KVNODE_OFFSET2CACHE(node1, head_offset);
    uint8_t *head2 = KVNODE_OFFSET2CACHE(node2, head_offset);

    if (keycmp(head1, head2, node2->value_offset - node2->head_offset - 1) != 0) {
        return NULL;
    }

    kvnode_t *del_node = NULL;

    // 版本号掉头, 1 > 255
    if (node1->version == 255 && node2->version == 1)
        del_node = node1;
    else if (node1->version == 1 && node2->version == 255)
        del_node = node2;
    else if (node1->version < node2->version)
        del_node = node1;
    else if (node1->version > node2->version)
        del_node = node2;

    if (del_node)
        kvnode_rm(del_node);

    return del_node;
}

struct find_node_t {
    int count;
    int found;
    const char *key;
    kvnode_t *ret_node;
};

/**
 * @brief  compare the kvnode by key
 * @param  [in] node
 * @param  [in] key
 * @return 0 on equal
 */
int kvnode_cmp_name(kvnode_t *node, const char *key)
{
    uint8_t *head =KVNODE_OFFSET2CACHE(node, head_offset);
    return keycmp((const uint8_t *)key, (uint8_t *)head,
                  node->value_offset - node->head_offset - 1);
}

int _iter_find(kvnode_t *node, void *p)
{
    struct find_node_t *n = (struct find_node_t *)p;

    if (kvnode_cmp_name(node, n->key) == 0) {
        n->found = 0;
        memcpy(n->ret_node, node, sizeof(kvnode_t));
        if (node->rw != 0)
            return -1;
    }
    n->count--;

    return n->count > 0 ? 0: -1;
}

/**
 * @brief  find the kvnode by key
 * @param  [in] block
 * @param  [in] key
 * @param  [in] node : used for store the result finding
 * @return 0 if find
 */
int kvblock_find(kvblock_t *block, const char *key, kvnode_t *node)
{
    struct find_node_t n;

    n.key = key;
    n.found = -1;
    n.count = block->count;
    n.ret_node = node;
    kvblock_iter(block, NODE_EXISTS, _iter_find, &n);

    return n.found;
}

int kvblock_iter(kvblock_t *block, int exists, int (*fn)(kvnode_t *, void *), void *data)
{
    kvnode_t node;

    kvblock_cache_malloc(block);

    uint8_t *next = block->mem_cache;
    while (kvblock_search(block, next, &node) == 0) {
        if (exists == NODE_ALL || NODE_VAILD(&node)) {
            int ret = fn(&node, data);
            if (ret != 0) {
                kvblock_cache_free(block);
                return ret;
            }
        }

        next = KVNODE_OFFSET2CACHE(&node, next_offset);
    }

    kvblock_cache_free(block);

    return 0;
}

static int _iter_rm_all_rw_node(kvnode_t *node, void *p)
{
    kvnode_rm(node);

    return 0;
}

/**
 * @brief  reset the block, detele valid kv pair
 * @param  [in] block
 * @return
 */
void kvblock_reset(kvblock_t *block)
{
    kvblock_iter(block, NODE_EXISTS, _iter_rm_all_rw_node, NULL);
}

static int _iter_calc(kvnode_t *node, void *i)
{
    if (NODE_VAILD(node)) {
        node->block->kv_size += node->next_offset - node->head_offset;
        node->block->count++;
        if (node->rw == 0)
            node->block->ro_count++;
#if (CONFIG_KV_ENABLE_CACHE || CONFIG_KV_START_OPT)
        {
            long idx;
            int valid = 0;
            cache_node_t *cache;
            kv_t *kv = node->block->kv;
            const char *key = (const char *)KVNODE_OFFSET2CACHE(node, head_offset);

            idx = (long)hash_get2(&kv->map, key, &valid);
            if (valid && idx >= 0) {
                /* conflict */
                lcache_node_t *lnode = (lcache_node_t*)calloc(1, sizeof(lcache_node_t));

                cache            = &lnode->cache;
                cache->block_id  = node->block->id;
                cache->offset    = node->head_offset;
                lnode->key       = strdup(key);
                kv->had_conflict = 1;
                slist_add_tail(&lnode->node, &kv->head);
                //printf("conflict error, key = %s\n", key);
            } else {
                idx = kv_cache_node_get(kv);
                if (idx >= 0) {
                    cache           = &kv->nodes[idx];
                    cache->block_id = node->block->id;
                    cache->offset   = node->head_offset;
                    hash_set(&kv->map, (const char *)KVNODE_OFFSET2CACHE(node, head_offset), (void*)idx);
                } else {
                    printf("error: iter cache node may be oom\n");
                }
            }
        }
#endif
    } else
        node->block->dirty_size += node->next_offset - node->head_offset;

    node->block->write_offset = align4(node->next_offset);

    return 0;
}

void kvblock_calc(kvblock_t *block)
{
    block->dirty_size = 0;
    block->kv_size    = 0;
    block->count      = 0;
    block->ro_count   = 0;
    kvblock_iter(block, NODE_ALL, _iter_calc, NULL);

    kvblock_cache_malloc(block);

    // calc write offset
    uint8_t *v = block->mem_cache +  block->write_offset;
    uint32_t magic = *(uint32_t*)(block->mem_cache + block->size - 4);

    while (v - block->mem_cache < 512 - 4) {
        if (*(uint32_t*)v != magic)
            block->write_offset = v - block->mem_cache + 4;
        v += 4;
    }

    kvblock_cache_free(block);
}

/**
 * @brief  show all kv to stdout in hex
 * @param  [in] block
 * @param  [in] num
 * @return
 */
void kvblock_show_data(kvblock_t *block, int num)
{
#ifdef CONFIG_NON_ADDRESS_FLASH
    uint32_t *v = (uint32_t*)malloc(block->size);
    if (v == NULL) {
        return;
    }
    block_read(block, 0, v, block->size);
#else
    uint32_t *v = (uint32_t *)block->mem;
#endif
    for (int j = 0; j < block->size / 4; j++) {
        printf("%08x ", v[j]);
        if (j % num == num - 1)
            printf("\n");
    }
#ifdef CONFIG_NON_ADDRESS_FLASH
    free(v);
#endif
}

static int _iter_dump(kvnode_t *node, void *p)
{
    kvnode_show(node);

    return 0;
}

/**
 * @brief  dump the block to stdout
 * @param  [in] block
 * @return
 */
void kvblock_dump(kvblock_t *block)
{
    // kvblock_show_data(block, 8);
    kvblock_iter(block, NODE_ALL, _iter_dump, NULL);
}

/**
 * @brief  malloc memory for the block
 * @param  [in] block
 * @return
 */
void kvblock_cache_malloc(kvblock_t *block)
{
#ifdef CONFIG_NON_ADDRESS_FLASH
    if (block->mem_cache == NULL) {
        block->mem_cache = (uint8_t*)malloc(block->size);
        if (block->mem_cache == NULL) {
            return;
        }
        block_read(block, 0, block->mem_cache, block->size);
    }
#else
    block->mem_cache = block->mem;
#endif
    block->mem_cache_ref ++;
}

/**
 * @brief  free memory for the block
 * @param  [in] block
 * @return
 */
void kvblock_cache_free(kvblock_t *block)
{
    block->mem_cache_ref --;

    if (block->mem_cache_ref == 0) {
        /* CONFIG_NON_ADDRESS_FLASH block->mem_cache != block->mem */
        if (block->mem_cache != block->mem) {
            free(block->mem_cache);
        }
        block->mem_cache = NULL;
    }
}
