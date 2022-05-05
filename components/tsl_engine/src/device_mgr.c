/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#include <tsl_engine/device_mgr.h>

#define dev_mgr_lock()   (aos_mutex_lock(&_mgr->lock, AOS_WAIT_FOREVER))
#define dev_mgr_unlock() (aos_mutex_unlock(&_mgr->lock))

static dev_mgr_t *_mgr;

/**
 * @brief  get the device-manager
 * @return NULL on error
 */
dev_mgr_t *dev_mgr_get_instance()
{
    if (!_mgr) {
        _mgr = aos_zalloc(sizeof(dev_mgr_t));

        slist_init(&_mgr->head);
        aos_mutex_new(&_mgr->lock);
        aos_queue_new(&_mgr->queue, _mgr->qbuf, sizeof(dev_msg_t) * MESSAGE_NUM, sizeof(dev_msg_t));
    }

    return _mgr;
}

/**
 * @brief  start the dev mgr
 * @return 0/-1
 */
int dev_mgr_start()
{
    //TODO:
    return 0;
}

/**
 * @brief  stop the dev mgr
 * @return 0/-1
 */
int dev_mgr_stop()
{
    //TODO:
    return 0;
}

/**
 * @brief  flush the dev mgr, will update dev status
 * @return 0/-1
 */
int dev_mgr_flush()
{
    //TODO:
    return 0;
}

/**
 * @brief  add device to the mgr dev-list
 * @param  [in] dev
 * @return 0/-1
 */
int dev_mgr_add_device(device_t *dev)
{
    int rc = -1, find = 0;
    device_t *dev_tmp;

    CHECK_PARAM(dev && dev->name && dev->type, rc);
    dev_mgr_lock();
    slist_for_each_entry(&_mgr->head, dev_tmp, device_t, node) {
        if (dev_tmp == dev) {
            find = 1;
            break;
        }
    }

    if (!find) {
        slist_add_tail(&dev->node, &_mgr->head);
        _mgr->cnt++;
        rc = 0;
    }
    dev_mgr_lock();

    return rc;
}

/**
 * @brief  remove device from the mgr dev-list
 * @param  [in] dev
 * @return 0/-1
 */
int dev_mgr_del_device(device_t *dev)
{
    int rc = -1;
    slist_t *tmp;
    device_t *dev_tmp;

    CHECK_PARAM(dev, rc);
    dev_mgr_lock();
    slist_for_each_entry_safe(&_mgr->head, tmp, dev_tmp, device_t, node) {
        if (dev_tmp == dev) {
            slist_del(&dev->node, &_mgr->head);
            _mgr->cnt--;
            rc = 0;
            break;
        }
    }
    dev_mgr_lock();

    return rc;
}

/**
 * @brief  find device by dev type and name
 * @param  [in] type
 * @param  [in] name
 * @return NULL on error
 */
device_t *dev_mgr_find_device(int type, const char *name)
{
    int find = 0;
    device_t *dev_tmp;

    CHECK_PARAM(type && name, NULL);
    dev_mgr_lock();
    slist_for_each_entry(&_mgr->head, dev_tmp, device_t, node) {
        if (dev_tmp->type == type && !strcmp(dev_tmp->name, name)) {
            find = 1;
            break;
        }
    }
    dev_mgr_lock();

    return find ? dev_tmp : NULL;
}


