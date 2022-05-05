/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <aos/list.h>
#include <aos/kernel.h>
#include <aos/kv.h>
#include <yoc/netio.h>
#include <yoc/fota.h>
#include <ulog/ulog.h>
#include <stdio.h>

#define TAG "fota"

typedef struct fota_netio_list {
    slist_t next;
    const fota_cls_t *cls;
} fota_cls_node_t;

static AOS_SLIST_HEAD(fota_cls_list);

int fota_register(const fota_cls_t *cls)
{
    fota_cls_node_t *node = aos_malloc(sizeof(fota_cls_node_t));

    if (node) {
        node->cls = cls;
        slist_add_tail(&node->next, &fota_cls_list);
        return 0;
    }

    return -1;
}

fota_t *fota_open(const char *fota_name, const char *dst, fota_event_cb_t event_cb)
{
    fota_cls_node_t *node;
    fota_t *fota = NULL;
    fota_config_t config = { 3000, 3000, 0, 30000, 0 };

    if (!(fota_name && dst)) {
        LOGE(TAG, "fota open e.");
        return NULL;
    }

    slist_for_each_entry(&fota_cls_list, node, fota_cls_node_t, next) {
        if (strcmp(node->cls->name, fota_name) == 0) {
            fota = aos_zalloc(sizeof(fota_t));
            if (fota == NULL) {
                LOGE(TAG, "malloc fota failed.");
                return NULL;
            }
            fota->to_path = strdup(dst);
            fota->cls = node->cls;
            fota->event_cb = event_cb;
            memcpy(&fota->config, &config, sizeof(fota_config_t));
            if (fota->cls->init)
                fota->cls->init(&fota->info);

            LOGD(TAG, "fota: 0x%x path:%s", fota, fota->to_path);
            break;
        }
    }

    if (fota == NULL) {
        LOGE(TAG, "fota open e. %s cls not found", fota_name);
        return NULL;
    }

    aos_sem_new(&fota->sem, 0);
    aos_sem_new(&fota->do_check_event, 0);
    aos_sem_new(&fota->sem_download, 0);
    return fota;
}

static int fota_version_check(fota_t *fota, fota_info_t *info) {
    if (fota && fota->cls && fota->cls->version_check) {
        int ret = fota->cls->version_check(info);
        if (ret != 0) {
            return -1;
        }
        if (fota->from_path == NULL) {
            fota->from_path = strdup(info->fota_url);
        } else {
            aos_free(fota->from_path);
            fota->from_path = strdup(info->fota_url);
        }
        LOGD(TAG, "get image url: %s", fota->from_path);
        return ret;
    }

    return -1;
}

static void fota_finish(fota_t *fota, fota_info_t *info)
{
    if (fota->cls->finish)
        fota->cls->finish(info);
}

static void fota_fail(fota_t *fota, fota_info_t *info)
{
    if (fota->cls->fail)
        fota->cls->fail(info);
}

static int fota_prepare(fota_t *fota)
{
    if (!(fota->from_path && fota->to_path)) {
        LOGE(TAG, "fota->from_path or fota->to_path is NULL");
        return -EINVAL;
    }
    LOGD(TAG, "###fota->from_path:%s, fota->to_path:%s", fota->from_path, fota->to_path);
    fota->buffer = aos_malloc(CONFIG_FOTA_BUFFER_SIZE);
    fota->from = netio_open(fota->from_path);
    fota->to = netio_open(fota->to_path);

    if (fota->buffer == NULL || fota->from == NULL || fota->to == NULL) {
        if (fota->buffer == NULL) {
            LOGD(TAG, "fota->buffer e");
        } else if (fota->from == NULL) {
            LOGD(TAG, "fota->from e");
        } else if (fota->to == NULL) {
            LOGD(TAG, "fota->to e");
        }
        goto error;
    }

    if (aos_kv_getint(KV_FOTA_OFFSET, &fota->offset) < 0) {
        if (aos_kv_setint(KV_FOTA_OFFSET, 0) < 0) {
            goto error;
        }
        fota->offset = 0;
    }

    LOGI(TAG, "FOTA seek %d", fota->offset);

    if (netio_seek(fota->from, fota->offset, SEEK_SET) != 0) {
        LOGD(TAG, "from seek error");
        goto error;
    }

    if (netio_seek(fota->to, fota->offset, SEEK_SET) != 0) {
        LOGD(TAG, "to seek error");
        goto error;
    }

    fota->status = FOTA_DOWNLOAD;
    fota->total_size = fota->from->size;
    LOGD(TAG, "fota prepare ok.");
    return 0;

error:
    if (fota->buffer) {
        aos_free(fota->buffer);
        fota->buffer = NULL;
    }
    if (fota->from) {
        netio_close(fota->from);
        fota->from = NULL;
    }
    if (fota->to) {
        netio_close(fota->to);
        fota->to = NULL;
    }

    return -1;
}

static void fota_release(fota_t *fota)
{
    LOGD(TAG, "%s,%d", __func__, __LINE__);
    if (fota->buffer) {
        aos_free(fota->buffer);
        fota->buffer = NULL;
    }

    if (fota->from) {
        netio_close(fota->from);
        fota->from = NULL;
    }

    if (fota->to) {
        netio_close(fota->to);
        fota->to = NULL;
    }
}

static void fota_task(void *arg)
{
    fota_t *fota = (fota_t *)arg;
    int retry = fota->config.retry_count;

    LOGD(TAG, "fota_task start: %s", fota->to_path);
    while (!fota->quit) {
        if (fota->status == FOTA_INIT) {
            LOGD(TAG, "fota_task FOTA_INIT! wait......");
            aos_sem_wait(&fota->do_check_event, -1);
            if (fota->quit) {
                break;
            }
            if (fota->status == FOTA_DOWNLOAD) {
                continue;
            }
            retry = fota->config.retry_count;
            if (fota->event_cb) {
                fota->error_code = FOTA_ERROR_NULL;
                fota->event_cb(arg, FOTA_EVENT_START);
            }

            if (fota_version_check(fota, &fota->info) == 0) {
                if (fota->event_cb) {
                    fota->error_code = FOTA_ERROR_NULL;
                    fota->event_cb(arg, FOTA_EVENT_VERSION);
                }
                if (fota->config.auto_check_en > 0) {
                    if (fota_prepare(fota) < 0) {
                        LOGE(TAG, "fota_prepare failed");
                        if (fota->event_cb) {
                            fota->error_code = FOTA_ERROR_PREPARE;
                            fota->event_cb(arg, FOTA_EVENT_VERSION);
                        }
                    }
                }
            } else {
                if (fota->event_cb) {
                    fota->error_code = FOTA_ERROR_VERSION_CHECK;
                    fota->event_cb(arg, FOTA_EVENT_VERSION);
                }
            }
            if (fota->config.auto_check_en > 0) {
                if (fota->status == FOTA_DOWNLOAD) {
                    aos_sem_signal(&fota->sem_download);
                    continue;
                }
                aos_msleep(fota->config.sleep_time);
                aos_sem_signal(&fota->do_check_event);
            }
        } else if (fota->status == FOTA_DOWNLOAD) {
            LOGD(TAG, "fota_task download! wait......");
            aos_sem_wait(&fota->sem_download, -1);
            if (fota->quit) {
                break;
            }

            int size = netio_read(fota->from, fota->buffer, CONFIG_FOTA_BUFFER_SIZE, fota->config.read_timeoutms);
            fota->total_size = fota->from->size;
            LOGD(TAG, "fota_task FOTA_DOWNLOAD! total:%d offset:%d", fota->from->size, fota->to->offset);
            LOGD(TAG, "##read: %d", size);
            if (size < 0) {
                LOGD(TAG, "read size < 0 %d", size);
                if (size == -2) {
                    LOGW(TAG, "reconnect again");
                    aos_sem_signal(&fota->sem_download);
                    continue;
                }
                if (fota->event_cb) {
                    fota->error_code = FOTA_ERROR_NET_READ;
                    fota->event_cb(arg, FOTA_EVENT_PROGRESS);
                }
                fota->status = FOTA_ABORT;
                LOGD(TAG, "fota abort");
                continue;
            } else if (size == 0) {
                // download finish
                LOGD(TAG, "read size 0.");
                if (fota->event_cb) {
                    fota->error_code = FOTA_ERROR_NULL;
                    fota->event_cb(arg, FOTA_EVENT_VERIFY);
                }
                int verify = fota_data_verify();
                fota_finish(fota, &fota->info);
                fota_release(fota);
                aos_kv_del(KV_FOTA_OFFSET);
                if (verify != 0) {
                    LOGE(TAG, "fota data verify failed.");
                    fota->error_code = FOTA_ERROR_VERIFY;
                    if (fota->event_cb)
                        fota->event_cb(arg, FOTA_EVENT_PROGRESS);
                    // goto init status
                    fota->status = FOTA_INIT;
                    if (fota->config.auto_check_en > 0) {
                        aos_msleep(fota->config.sleep_time);
                        aos_sem_signal(&fota->do_check_event);
                    }
                } else {
                    LOGD(TAG, "fota data verify ok.");
#ifdef CONFIG_DL_FINISH_FLAG_POWSAVE
                    aos_kv_setint(KV_FOTA_FINISH, 1);
#endif
                    fota->status = FOTA_FINISH;
                    fota->error_code = FOTA_ERROR_NULL;
                    if (fota->event_cb)
                        fota->event_cb(arg, FOTA_EVENT_FINISH);
                    if (fota->config.auto_check_en > 0) {
                        fota_restart(fota, 0);
                    }
                }
                continue;
            }
#ifdef CONFIG_DL_FINISH_FLAG_POWSAVE
            if (fota->offset == 0) {
                LOGD(TAG, "set fota_finish to 0");
                if (aos_kv_setint(KV_FOTA_FINISH, 0) < 0) {
                    goto write_err;
                }
            }
#endif
            size = netio_write(fota->to, fota->buffer, size, fota->config.write_timeoutms);
            LOGI(TAG, "write size: %d", size);
            if (size > 0) {
                if (aos_kv_setint(KV_FOTA_OFFSET, fota->offset + size) < 0) {
                    goto write_err;
                }
                fota->offset += size;
                if (fota->event_cb) {
                    fota->error_code = FOTA_ERROR_NULL;
                    fota->event_cb(arg, FOTA_EVENT_PROGRESS);
                }
                aos_sem_signal(&fota->sem_download);
            } else {
write_err:
                // flash write error
                LOGE(TAG, "flash write size error.");
                if (fota->event_cb) {
                    fota->error_code = FOTA_ERROR_WRITE;
                    fota->event_cb(arg, FOTA_EVENT_PROGRESS);
                }
                fota->status = FOTA_ABORT;
            }
        } else if (fota->status == FOTA_ABORT) {
            LOGD(TAG, "fota_task FOTA_ABORT!");
            if (retry != 0) {
                LOGW(TAG, "fota retry: %d!", retry);
                retry--;
                fota->status = FOTA_DOWNLOAD;
                aos_msleep(fota->config.sleep_time);
                aos_sem_signal(&fota->sem_download);
            } else {
                retry = fota->config.retry_count;
                fota_fail(fota, &fota->info);
                fota_release(fota);
                fota->status = FOTA_INIT;
                if (fota->config.auto_check_en > 0) {
                    aos_msleep(fota->config.sleep_time);
                    aos_sem_signal(&fota->do_check_event);
                }
            }
        } else {
            // finish state will come here.
            LOGD(TAG, "##fota status:%d", fota->status);
            aos_msleep(fota->config.sleep_time);
        }
    }

    LOGD(TAG, "force quit need release source");
    fota->status = 0; // need reset for next restart
    fota_release(fota);

    if (fota->event_cb) {
        fota->error_code = FOTA_ERROR_NULL;
        fota->event_cb(arg, FOTA_EVENT_QUIT);
    }

    /* will free fota */
    aos_sem_signal(&fota->sem);
}

void fota_do_check(fota_t *fota)
{
    LOGD(TAG, "fota do check signal........");
    if (fota && (fota->status == FOTA_INIT || fota->status == FOTA_FINISH)) {
        fota->status = FOTA_INIT;
        aos_sem_signal(&fota->do_check_event);
    }
}

int fota_start(fota_t *fota)
{
    if (fota == NULL) {
        return -EINVAL;
    }

    if (fota->status == 0) {
        fota->status = FOTA_INIT;
        fota->quit = 0;
#ifdef CONFIG_DL_FINISH_FLAG_POWSAVE
        int isfinish;
        if (aos_kv_getint(KV_FOTA_FINISH, &isfinish) < 0) {
            isfinish = 0;
        }
        if (isfinish > 0) {
            LOGD(TAG, "come to fota finish state.");
            fota->status = FOTA_FINISH;
        }
#endif
        if (aos_task_new_ext(&fota->task, "fota", fota_task, fota, CONFIG_FOTA_TASK_STACK_SIZE, 45) != 0) {
            fota->quit = 1;
            fota->status = 0;
            LOGE(TAG, "fota task create failed.");
            return -1;
        }
        if (fota->config.auto_check_en > 0) {
            fota_do_check(fota);
        }
    }

    return 0;
}

int fota_stop(fota_t *fota)
{
    if (fota == NULL) {
        return -EINVAL;
    }
    LOGD(TAG, "%s,%d", __func__, __LINE__);

    fota->quit = 1;
    aos_sem_signal(&fota->do_check_event);
    aos_sem_signal(&fota->sem_download);

    return 0;
}

int fota_close(fota_t *fota)
{
    if (fota == NULL) {
        return -EINVAL;
    }
    LOGD(TAG, "%s,%d", __func__, __LINE__);

    fota->quit = 1;
    aos_sem_signal(&fota->do_check_event);
    aos_sem_signal(&fota->sem_download);
    aos_sem_wait(&fota->sem, -1);
    aos_sem_free(&fota->sem);
    aos_sem_free(&fota->do_check_event);
    aos_sem_free(&fota->sem_download);

    if (fota->from_path) aos_free(fota->from_path);
    if (fota->to_path) aos_free(fota->to_path);
    if (fota->buffer) aos_free(fota->buffer);
    if (fota->from) netio_close(fota->from);
    if (fota->to) netio_close(fota->to);

    aos_free(fota);

    return 0;
}

int fota_download(fota_t *fota)
{
    if (fota == NULL) {
        LOGE(TAG, "fota param null.");
        return -EINVAL;
    }
    LOGD(TAG, "fota download, status:%d", fota->status);
    if (fota->status == FOTA_ABORT || fota->status == FOTA_INIT || fota->status == FOTA_FINISH) {
        if (fota->status == FOTA_INIT || fota->status == FOTA_FINISH) {
            if (fota_prepare(fota)) {
                LOGE(TAG, "fota_prepare failed");
                if (fota->event_cb) {
                    fota->error_code = FOTA_ERROR_PREPARE;
                    fota->event_cb(fota, FOTA_EVENT_VERSION);
                }
                return -1;
            }
            aos_sem_signal(&fota->do_check_event);
        } else {
            fota->status = FOTA_DOWNLOAD;
        }
        LOGD(TAG, "signal download.");
        aos_sem_signal(&fota->sem_download);
        return 0;
    }
    LOGW(TAG, "the status is not allow to download.");
    return -1;
}

static void timer_thread(void *timer, void *args)
{
    fota_t *fota = (fota_t *)args;
    LOGD(TAG, "timer_thread, fota:0x%x", fota);
    if (fota) {
        LOGD(TAG, "report restart event.");
        if (fota->event_cb) {
            fota->error_code = FOTA_ERROR_NULL;
            fota->event_cb(fota, FOTA_EVENT_RESTART);
        }
        aos_timer_stop(&fota->restart_timer);
        aos_timer_free(&fota->restart_timer);
        LOGD(TAG, "stop and delete timer ok.");
        return;
    }
    LOGE(TAG, "timer task params error.");
}

int fota_restart(fota_t *fota, int delay_ms)
{
    if (fota == NULL) {
        LOGE(TAG, "fota param null.");
        return -EINVAL;
    }
    LOGD(TAG, "%s,%d, delay_ms:%d", __func__, __LINE__, delay_ms);
    if (fota->status != FOTA_FINISH) {
        LOGE(TAG, "fota status is not FOTA_FINISH, cant restart to upgrade.");
        return -1;
    }
    if (fota->cls && fota->cls->restart) {
        if (delay_ms > 0) {
            int ret = aos_timer_new_ext(&fota->restart_timer, timer_thread, fota, delay_ms, 0, 1);
            if (ret < 0) {
                LOGE(TAG, "timer_create error [%d]!\n", errno);
                return -1;
            }
            LOGD(TAG, "set timer ok.");
            return 0;
        }
#ifdef CONFIG_DL_FINISH_FLAG_POWSAVE
        if (aos_kv_setint(KV_FOTA_FINISH, 0) < 0) {
            LOGE(TAG, "set finish 0 error.");
            return -1;
        }
#endif
        fota->cls->restart();
    }
    return 0;
}

void fota_config(fota_t *fota, fota_config_t *config)
{
    if (!(fota && config)) {
        LOGE(TAG, "fota or config param null.");
        return;
    }
    memcpy(&fota->config, config, sizeof(fota_config_t));
}

fota_config_t *fota_get_config(fota_t *fota)
{
    if (fota == NULL) {
        LOGE(TAG, "fota param null.");
        return NULL;
    }
    return &fota->config;
}

void fota_set_auto_check(fota_t *fota, int enable)
{
    if (fota)
        fota->config.auto_check_en = enable;
}

int fota_get_auto_check(fota_t *fota)
{
    if (fota)
        return fota->config.auto_check_en;
    return 0;
}

fota_status_e fota_get_status(fota_t *fota)
{
    if (fota == NULL) {
        LOGE(TAG, "fota param null.");
        return 0;
    }
    return fota->status;
}
