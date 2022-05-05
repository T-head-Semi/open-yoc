/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#include <tsl_engine/device.h>

#define dev_lock()   (aos_mutex_lock(&dev->lock, AOS_WAIT_FOREVER))
#define dev_unlock() (aos_mutex_unlock(&dev->lock))

static struct {
    slist_t            head;
} _dev_clses;

/**
 * @brief  regist device class
 * @param  [in] cls
 * @return 0/-1
 */
int dev_cls_register(const device_t *cls)
{
    device_t *dev_cls;
    int rc = -1, find = 0;

    if (cls && cls->name && cls->size && cls->ops && cls->type != DEV_TYPE_UNKNOWN) {
        slist_for_each_entry(&_dev_clses.head, dev_cls, device_t, node) {
            if (dev_cls->type == cls->type) {
                find = 1;
                break;
            }
        }

        if (!find) {
            dev_cls = aos_malloc(cls->size);
            memcpy(dev_cls, cls, cls->size);
            slist_add_tail(&dev_cls->node, &_dev_clses.head);
            rc = 0;
        }

    }

    return rc;
}

static device_t *_dev_cls_find(int type)
{
    device_t *dev_cls;

    slist_for_each_entry(&_dev_clses.head, dev_cls, device_t, node) {
        if (dev_cls->type == type) {
            return dev_cls;
        }
    }

    return NULL;
}

/**
 * @brief  alloc a device by dev_type
 * @param  [in] type
 * @param  [in] name
 * @return NULL on error
 */
device_t *dev_new(int type, const char *name)
{
    device_t *dev = NULL, *cls;

    CHECK_PARAM(type && name, NULL);
    cls = _dev_cls_find(type);
    if (cls) {
        dev = aos_malloc(cls->size);
        memcpy(dev, cls, cls->size);
        aos_mutex_new(&dev->lock);
        hash_init(&dev->map, 8);
        dev->name = strdup(name);
    }

    return dev;
}

/**
 * @brief  config the device
 * @param  [in] dev
 * @param  [in] conf
 * @return 0/-1
 */
int dev_config(device_t *dev, const dev_conf_t *conf)
{
    int rc = -1;

    CHECK_PARAM(dev && conf, rc);
    CHECK_PARAM(conf->mac, rc);
    dev_lock();
    memcpy(dev->mac, conf->mac, 6);
    rc = 0;
    dev_unlock();

    return rc;
}

/**
 * @brief  add a key-value pair to the dev kv-map
 * @param  [in] dev
 * @param  [in] key
 * @param  [in] data : value of the key is a pointer, may be need free by the caller after free
 * @return 0/-1
 */
int dev_kv_set(device_t *dev, const char *key, void *data)
{
    int rc;

    CHECK_PARAM(dev && key, -1);
    dev_lock();
    rc = hash_set(&dev->map, key, data);
    dev_unlock();

    return rc;
}

/**
 * @brief  get value from dev kv-map by the key
 * @param  [in] dev
 * @param  [in] key
 * @return
 */
void *dev_kv_get(device_t *dev, const char *key)
{
    void *value;

    CHECK_PARAM(dev && key, NULL);
    dev_lock();
    value = hash_get(&dev->map, key);
    dev_unlock();

    return value;
}

/**
 * @brief  device ioctl interface
 * @param  [in] dev
 * @param  [in] cmd
 * @param  [in] arg
 * @param  [in] arg_size
 * @return 0/-1
 */
int dev_ioctl(device_t *dev, int cmd, void *arg, size_t *arg_size)
{
    int rc = -1;

    CHECK_PARAM(dev && cmd, rc);
    dev_lock();
    rc = dev->ops->ioctl(dev, cmd, arg, arg_size);
    dev_unlock();

    return rc;
}

/**
 * @brief  open the device
 * @param  [in] dev
 * @return 0/-1
 */
int dev_open(device_t *dev)
{
    int rc;

    CHECK_PARAM(dev, -1);
    if (dev) {
        dev_lock();
        rc = dev->ops->open(dev);
        dev_unlock();
    }

    return rc;
}

/**
 * @brief  send data to the real dev
 * @param  [in] dev
 * @param  [in] data
 * @param  [in] size
 * @return -1 on error
 */
int dev_send_to_device(device_t *dev, const uint8_t *data, size_t size)
{
    int rc;

    CHECK_PARAM(dev && data && size, -1);
    dev_lock();
    rc = dev->ops->send_to_device(dev, data, size);
    dev_unlock();

    return rc;
}

/**
 * @brief  send data to the tsl-cloud
 * @param  [in] dev
 * @param  [in] data
 * @param  [in] size
 * @return -1 on error
 */
int dev_send_to_cloud(device_t *dev, const uint8_t *data, size_t size)
{
    int rc;

    CHECK_PARAM(dev && data && size, -1);
    dev_lock();
    rc = dev->ops->send_to_cloud(dev, data, size);
    dev_unlock();

    return rc;
}

/**
 * @brief  close the device
 * @param  [in] dev
 * @return
 */
void dev_close(device_t *dev)
{
    //TODO:
    if (dev) {
        dev_lock();
        dev->ops->close(dev);
        dev_unlock();
    }
}

/**
 * @brief  free the device
 * @param  [in] dev
 * @return
 */
void dev_free(device_t *dev)
{
    if (dev) {
        aos_mutex_free(&dev->lock);
        hash_uninit(&dev->map);
        aos_free(dev->name);
        aos_free(dev);
    }
}


