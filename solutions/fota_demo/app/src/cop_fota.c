/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <aos/aos.h>
#include <aos/kv.h>
#include <yoc/fota.h>
#include <yoc/sysinfo.h>
#include <uservice/eventid.h>
#include <uservice/uservice.h>
#include <cJSON.h>
#include <posix/timer.h>
#include "app_main.h"

#define KV_FOTA_CYCLE_MS        "fota_cycle"
#define KV_FOTA_START_MS        "fota_start"
#define KV_FOTA_ENABLED         "fota_en"
#define FOTA_CYCLE_DELAY_TIME   (20000)
#define FOTA_START_DELAY_TIME   (0)

#define TAG "app_fota"

int g_fota_delay_timer = 2500;
static fota_t *g_fota_handle = NULL;

void fota_user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    if (event_id == EVENT_FOTA_START) {
        fota_start(g_fota_handle);
    }
    else {
        ;
    }
}

struct timespec diff_timespec(struct timespec start, struct timespec end)
{
     struct timespec result;
 
     if (end.tv_nsec < start.tv_nsec)
     { 
        result.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;        
        result.tv_sec = end.tv_sec - 1 - start.tv_sec;
     }
     else
     {
        result.tv_nsec = end.tv_nsec - start.tv_nsec;        
        result.tv_sec = end.tv_sec - start.tv_sec;
     }
 
    return result;
}

static int fota_event_cb(void *arg, fota_event_e event)
{
    static int data_offset;
    static struct timespec tv;
    fota_t *fota = (fota_t *)arg;

    switch (event) {
        case FOTA_EVENT_START:
            LOGD(TAG, "FOTA START :%x", fota->status);
            break;
        case FOTA_EVENT_VERSION:
        {
            LOGD(TAG, "FOTA VERSION CHECK :%x", fota->status);
            data_offset = 0;
            memset(&tv, 0, sizeof(struct timespec));
            cJSON *root = cJSON_CreateObject();
            if (fota->error_code != FOTA_ERROR_NULL) {
                cJSON_AddNumberToObject(root, "code", 1);
                cJSON_AddStringToObject(root, "msg", "version check failed!");
            } else {
                cJSON_AddNumberToObject(root, "code", 0);
                cJSON_AddStringToObject(root, "curversion", fota->info.cur_version);
                cJSON_AddStringToObject(root, "newversion", fota->info.new_version);
                cJSON_AddNumberToObject(root, "timestamp", fota->info.timestamp);
                cJSON_AddStringToObject(root, "changelog", fota->info.changelog);
                cJSON_AddStringToObject(root, "local_changelog", fota->info.local_changelog);
            }
            char *out = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);
            if (out != NULL) {
                LOGD(TAG, out);
                cJSON_free(out);
            }
            break;
        }
        case FOTA_EVENT_PROGRESS:
        {
            LOGD(TAG, "FOTA PROGRESS :%x, %d, %d", fota->status, fota->offset, fota->total_size);
            int64_t cur_size = fota->offset;
            int64_t total_size = fota->total_size;
            int speed = 0; //kbps
            int percent = 0;
            if (total_size > 0) {
                percent = (int)(cur_size * 100 / total_size);
            }
            cJSON *root = cJSON_CreateObject();
            if (fota->error_code != FOTA_ERROR_NULL) {
                cJSON_AddNumberToObject(root, "code", 1);
                if (fota->error_code == FOTA_ERROR_VERIFY) {
                    cJSON_AddStringToObject(root, "msg", "fota image verify failed!");
                } else {
                    cJSON_AddStringToObject(root, "msg", "download failed!");
                }
                cJSON_AddNumberToObject(root, "total_size", total_size);
                cJSON_AddNumberToObject(root, "cur_size", cur_size);
                cJSON_AddNumberToObject(root, "percent", percent);
            } else {
                // current_size, total_size, percent, speed
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                if (tv.tv_sec > 0 && tv.tv_nsec > 0 && data_offset > 0) {
                    struct timespec diff = diff_timespec(tv, now);
                    int millisecond = diff.tv_sec * 1000 + diff.tv_nsec / 1000000;
                    LOGD(TAG, "interval time: %d ms", millisecond);
                    speed = ((fota->offset - data_offset) / 1024) * 1000 / millisecond;
                }
                cJSON_AddNumberToObject(root, "code", 0);
                cJSON_AddNumberToObject(root, "total_size", total_size);
                cJSON_AddNumberToObject(root, "cur_size", cur_size);
                cJSON_AddNumberToObject(root, "percent", percent);
                cJSON_AddNumberToObject(root, "speed", speed);
                tv.tv_sec = now.tv_sec;
                tv.tv_nsec = now.tv_nsec;
                data_offset = fota->offset;
            }
            char *out = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);
            if (out != NULL) {
                LOGD(TAG, out);
                cJSON_free(out);
            }
            break;            
        }
            
        case FOTA_EVENT_FAIL:
            LOGD(TAG, "FOTA FAIL :%x, %d", fota->status, fota->error_code);
            break;
        case FOTA_EVENT_VERIFY:
            LOGD(TAG, "FOTA VERIFY :%x", fota->status);
            break;
        case FOTA_EVENT_FINISH:
            LOGD(TAG, "FOTA FINISH :%x", fota->status);
            // reboot to upgrade by bootloader;
            aos_reboot();
            break;
        case FOTA_EVENT_RESTART:
            LOGD(TAG, "FOTA RESTART :%x", fota->status);
            break;
        case FOTA_EVENT_QUIT:
            LOGD(TAG, "FOTA QUIT :%x", fota->status);
            break;
        default:
            break;
    }
    return 0;
}

void app_fota_init(void)
{
    int ret;
    int fota_en = 1;

    LOGI(TAG, "ver=============> %s", aos_get_app_version());
    LOGI(TAG, "deviceid========> %s", aos_get_device_id());
    LOGI(TAG, "model===========> %s", aos_get_product_model());
    ret = aos_kv_getint(KV_FOTA_ENABLED, &fota_en);
    if (ret == 0 && fota_en == 0) {
        return;
    }

    fota_register_cop();
    netio_register_httpc(NULL);
    netio_register_flash();
    g_fota_handle = fota_open("cop", "flash://misc", fota_event_cb);

    ret = aos_kv_getint(KV_FOTA_CYCLE_MS, &(g_fota_handle->config.sleep_time));
    if (ret != 0 || g_fota_handle->config.sleep_time < 1000) {
        g_fota_handle->config.sleep_time = FOTA_CYCLE_DELAY_TIME;
    }
    g_fota_handle->config.read_timeoutms = 20000;
    g_fota_handle->config.write_timeoutms = 20000;
    g_fota_handle->config.retry_count = 0;
    g_fota_handle->config.auto_check_en = 1;

    ret = aos_kv_getint(KV_FOTA_START_MS, &g_fota_delay_timer);
    if (ret != 0) {
        g_fota_delay_timer = FOTA_START_DELAY_TIME;
    }
    event_subscribe(EVENT_FOTA_START, fota_user_local_event_cb, NULL);
}

void app_fota_start(void)
{
    int ret;
    int fota_en = 1;
    ret = aos_kv_getint(KV_FOTA_ENABLED, &fota_en);
    if (ret == 0 && fota_en == 0) {
        return;
    }

    event_publish_delay(EVENT_FOTA_START, NULL, g_fota_delay_timer);
}

int app_fota_is_running(void)
{
    if (g_fota_handle->status == FOTA_DOWNLOAD) {
        LOGD(TAG, "fota is running..........\n");
        return 1;
    }
    return 0;
}

int app_fota_is_downloading(void)
{
    if (g_fota_handle == NULL) {
        return 0;
    }

    if (fota_get_status(g_fota_handle) == FOTA_DOWNLOAD) {
        return 1;
    }
    return 0;
}