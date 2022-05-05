/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_STANDALONE_NVRAM) && CONFIG_STANDALONE_NVRAM
#include <aos/kv.h>
#include <aos/debug.h>

#include <devices/flash.h>
#include <yoc/partition.h>
#include <aos/debug.h>

#include <kvset.h>

static int __kv_init(kv_t *kv, const char *partition);
static int __kv_setdata(char *key, char *buf, int bufsize);
static int __kv_getdata(char *key, char *buf, int bufsize);
static int __kv_del(char *key);
static int __kv_reset(void);

/*************************
 * Set Get API
 *************************/

int nvram_get_val(const char *key, char *value, int len)
{
    int ret;
    ret = __kv_getdata((char *)key, (char *)value, len - 1);

    if(ret > 0) {
        value[ret < len ? ret : len - 1] = '\0';
    }

    return ret;
}

int nvram_set_val(const char *key, char *value)
{
    return __kv_setdata((char *)key, (void *)value, strlen(value));
}

int nvram_del(const char *key)
{
    return __kv_del((char *)key);
}

int nvram_reset(void)
{
    return __kv_reset();
}

/*************************
 * KV Instance
 *************************/
static kv_t g_kvfct;
static aos_mutex_t kv_lock;

int nvram_init(const char *partname)
{
    aos_mutex_new(&kv_lock);
    return __kv_init(&g_kvfct, partname);
}

static int kv_partition_erase(kv_t *kv, int pos, int size)
{
    return partition_erase(kv->handle, pos, 1);
}

static int kv_partition_write(kv_t *kv, int pos, void *data, int size)
{
    return partition_write(kv->handle, pos, data, size);
}

static int kv_partition_read(kv_t *kv, int pos, void *data, int size)
{
    return partition_read(kv->handle, pos, data, size);
}

static flash_ops_t partition_ops = {
    .write = kv_partition_write,
    .erase = kv_partition_erase,
    .read =  kv_partition_read
};

static int __kv_init(kv_t *kv, const char *partition)
{
    memset(kv, 0, sizeof(kv_t));
    kv->handle = partition_open(partition);
    kv->ops    = &partition_ops;

    if (kv->handle >= 0) {
        partition_info_t *lp = hal_flash_get_info(kv->handle);
        aos_assert(lp);

        uint8_t *mem        = (uint8_t *)(lp->start_addr + lp->base_addr);
        int      block_size = lp->sector_size;
        int      block_num  = lp->length / lp->sector_size;

        kv_init(kv, mem, block_num, block_size);

        return 0;
    }

    return -1;
}

static int __kv_setdata(char *key, char *buf, int bufsize)
{
    if (g_kvfct.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_set(&g_kvfct, key, buf, bufsize) >= 0 ? 0 : -1;
    aos_mutex_unlock(&kv_lock);

    return ret;
}

static int __kv_getdata(char *key, char *buf, int bufsize)
{
    if (g_kvfct.handle < 0) {
        return -1;
    }

    if (key == NULL || buf == NULL || bufsize <= 0)
        return -1;
    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_get(&g_kvfct, key, buf, bufsize);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

static int __kv_del(char *key)
{
    if (g_kvfct.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_rm(&g_kvfct, key);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

static int __kv_reset(void)
{
    if (g_kvfct.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_reset(&g_kvfct);
    aos_mutex_unlock(&kv_lock);

    return ret;
}
#endif
