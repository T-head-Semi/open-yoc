/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <aos/kv.h>
#include <aos/debug.h>

#include <devices/flash.h>
#include <yoc/partition.h>
#include <aos/debug.h>

#include <kvset.h>

extern int __kv_setdata(char *key, char *buf, int bufsize);
extern int __kv_getdata(char *key, char *buf, int bufsize);
extern int __kv_del(char *key);
extern int __kv_reset(void);

int aos_kv_set(const char *key, void *value, int len, int sync)
{
    return __kv_setdata((char *)key, value, len);
}

int aos_kv_setstring(const char *key, const char *v)
{
    return __kv_setdata((char *)key, (void *)v, strlen(v));
}

int aos_kv_setfloat(const char *key, float v)
{
    return __kv_setdata((char *)key, (void *)&v, sizeof(v));
}

int aos_kv_setint(const char *key, int v)
{
    return __kv_setdata((char *)key, (void *)&v, sizeof(v));
}

int aos_kv_get(const char *key, void *buffer, int *buffer_len)
{
    if (buffer_len) {
        int ret = __kv_getdata((char *)key, buffer, *buffer_len);

        if (ret > 0) {
            *buffer_len = ret;
        }

        return (ret <= 0) ? -1 : 0;
    }

    return -1;
}

int aos_kv_getstring(const char *key, char *value, int len)
{
    int ret;
    ret = __kv_getdata((char *)key, (char *)value, len - 1);

    if(ret > 0) {
        value[ret < len ? ret : len - 1] = '\0';
    }

    return ret;
}

int aos_kv_getfloat(const char *key, float *value)
{
    int ret = __kv_getdata((char *)key, (char *)value, sizeof(float));
    return (ret == sizeof(float)) ? 0 : -1;
}

int aos_kv_getint(const char *key, int *value)
{
    int ret = __kv_getdata((char *)key, (char *)value, sizeof(int));
    return (ret == sizeof(int)) ? 0 : -1;
}

int aos_kv_del(const char *key)
{
    return __kv_del((char *)key);
}

int aos_kv_reset(void)
{
    return __kv_reset();
}

static kv_t g_kv;

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

static int kv_flash_erase(kv_t *kv, int pos, int size)
{
    int ret;
    aos_dev_t *dev = (aos_dev_t *)((long)kv->handle);
    flash_dev_info_t flash_info;

    flash_get_info(dev, &flash_info);

    int offset = pos + (long)kv->mem - flash_info.start_addr;

    ret = flash_erase(dev, offset, 1);

    return ret;
}

static int kv_flash_write(kv_t *kv, int pos, void *data, int size)
{
    int ret;
    aos_dev_t *dev = (aos_dev_t *)((long)kv->handle);
    flash_dev_info_t flash_info;

    flash_get_info(dev, &flash_info);

    int offset = pos + (long)kv->mem - flash_info.start_addr;
    ret = flash_program(dev, offset, data, size);

    return ret;
}

static int kv_flash_read(kv_t *kv, int pos, void *data, int size)
{
    int ret;
    aos_dev_t *dev = (aos_dev_t *)((long)kv->handle);
    flash_dev_info_t flash_info;

    flash_get_info(dev, &flash_info);

    int offset = pos + (long)kv->mem - flash_info.start_addr;
    ret = flash_read(dev, offset, data, size);
    return ret;
}

static flash_ops_t flash_ops = {
    .write = kv_flash_write,
    .erase = kv_flash_erase,
    .read = kv_flash_read
};

int kv2x_init(kv_t *kv, const char *partition)
{
    memset(kv, 0, sizeof(kv_t));
    kv->handle = partition_open(partition);
    kv->ops    = &partition_ops;

    if (kv->handle >= 0) {
        partition_info_t *lp = hal_flash_get_info(kv->handle);
        aos_assert(lp);

        uint8_t *mem        = (uint8_t *)((unsigned long)(lp->start_addr + lp->base_addr));
        int      block_size = lp->sector_size;
        int      block_num  = lp->length / lp->sector_size;

        return kv_init(kv, mem, block_num, block_size);
    }

    return -1;
}

int kv2x_flash_init(kv_t *kv, const char *flashname, int start, int num)
{
    memset(kv, 0, sizeof(kv_t));
    kv->handle = (long)flash_open(flashname);
    kv->ops    = &flash_ops;

    if (kv->handle >= 0) {
        flash_dev_info_t flash_info;

        flash_get_info((aos_dev_t *)((long)kv->handle), &flash_info);

        uint8_t *mem        = (uint8_t *)((long)start);
        int      block_size = flash_info.block_size;
        int      block_num  = num;

        kv_init(kv, mem, block_num, block_size);

        return 0;
    }

    return -1;
}

static aos_mutex_t kv_lock;
int aos_kv_init(const char *partname)
{
    aos_mutex_new(&kv_lock);
    return kv2x_init(&g_kv, partname);
}

int aos_kv_flash_init(const char *flashname, int start, int block_num)
{
    aos_mutex_new(&kv_lock);
    return kv2x_flash_init(&g_kv, flashname, start, block_num);
}

int __kv_setdata(char *key, char *buf, int bufsize)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_set(&g_kv, key, buf, bufsize) >= 0 ? 0 : -1;
    aos_mutex_unlock(&kv_lock);

    return ret;
}

int __kv_getdata(char *key, char *buf, int bufsize)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    if (key == NULL || buf == NULL || bufsize <= 0)
        return -1;
    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_get(&g_kv, key, buf, bufsize);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

int __kv_del(char *key)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_rm(&g_kv, key);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

int __kv_reset(void)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_reset(&g_kv);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

void __kv_dump()
{
    if (g_kv.handle < 0) {
        return;
    }

    aos_mutex_lock(&kv_lock, -1);
    kv_dump(&g_kv);
    aos_mutex_unlock(&kv_lock);
}

void __show_data()
{
    if (g_kv.handle < 0) {
        return;
    }

    aos_mutex_lock(&kv_lock, -1);
    kv_show_data(&g_kv);
    aos_mutex_unlock(&kv_lock);
}

static int _iter_list(kvnode_t *node, void *p)
{
    printf("%s: %s\n", \
           KVNODE_OFFSET2CACHE(node, head_offset),
           KVNODE_OFFSET2CACHE(node, value_offset)
          );
    return 0;
}

struct kv_foreach_func_t {
    void (*func)(char *key, char *val, uint16_t val_size, void *arg);
    void *arg;
};

static int _iter_foreach(kvnode_t *node, void *p)
{
    struct kv_foreach_func_t *func = p;

    if (func && func->func) {
        func->func((char *)KVNODE_OFFSET2CACHE(node, head_offset), (char *)KVNODE_OFFSET2CACHE(node, value_offset), node->val_size, func->arg);
    }
    return 0;
}

void __kv_foreach(void (*func)(char *key, char *val, uint16_t val_size, void *arg), void *arg)
{
    if (g_kv.handle < 0) {
        return;
    }

    struct kv_foreach_func_t cb_func = {0};
    cb_func.func = func;
    cb_func.arg = arg;

    aos_mutex_lock(&kv_lock, -1);
    kv_iter(&g_kv, _iter_foreach, &cb_func);
    aos_mutex_unlock(&kv_lock);
}

void __kv_list()
{
    if (g_kv.handle < 0) {
        return;
    }

    aos_mutex_lock(&kv_lock, -1);
    kv_iter(&g_kv, _iter_list, NULL);
    aos_mutex_unlock(&kv_lock);
}

#if !defined(CONFIG_STANDALONE_NVRAM) || !CONFIG_STANDALONE_NVRAM

int nvram_get_val(const char *key, char *buf, int size)
{
    int ret;
    ret = __kv_getdata((char *)key, (char *)buf, size - 1);

    if(ret > 0) {
        buf[ret < size ? ret : size - 1] = '\0';
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

#endif
