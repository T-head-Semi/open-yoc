/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include <aos/kv.h>

#include "devices/wifi.h"
#include "app_main.h"

#define TAG "APP_WIFI"

#define WIFI_CONFIG_NUM_MAX             5
#define WIFI_CONFIG_NUM_MAX_AP          15
#define WIFI_CONFIG_NUM_CUR_KEY         "wifi_cur"
#define WIFI_CONFIG_SSID_KEY_PATTERN    "wifi_ssid%d"
#define WIFI_CONFIG_PSK_KEY_PATTERN     "wifi_psk%d"

typedef struct app_wifi_config {
    char    ssid[WIFI_CONFIG_NUM_MAX][MAX_SSID_SIZE + 1];
    char    psk[WIFI_CONFIG_NUM_MAX][MAX_PASSWD_SIZE + 1];
    char    ap_ssid[WIFI_CONFIG_NUM_MAX_AP][MAX_PASSWD_SIZE + 1];
    int     cur_used_ssid;
    uint32_t empty_ssid_table;
    int ap_num;
    int max_num;
} app_wifi_config_t ;

static app_wifi_config_t g_wifi_config;
static aos_mutex_t wifi_config_mutex;

static void wifi_config_init(app_wifi_config_t *config)
{
    memset(config, 0, sizeof(app_wifi_config_t));
    int32_t i;
    int ret = -1;
    char wifi_ssid_key[32];
    char wifi_psk_key[32];

    config->max_num = WIFI_CONFIG_NUM_MAX;

    ret = aos_mutex_new(&wifi_config_mutex);
    if (ret != 0) {
        return ;
    }

    for (i = 0; i < config->max_num; i++) {
        snprintf(wifi_ssid_key, sizeof(wifi_ssid_key), WIFI_CONFIG_SSID_KEY_PATTERN, i);
        if (aos_kv_getstring(wifi_ssid_key, config->ssid[i], sizeof(config->ssid[i])) < 0) {
            // config->max_num = i;
            config->empty_ssid_table |= (1 << i);
            continue;
        }

        snprintf(wifi_psk_key, sizeof(wifi_psk_key), WIFI_CONFIG_PSK_KEY_PATTERN, i);
        aos_kv_getstring(wifi_psk_key, config->psk[i], sizeof(config->psk[i]));
        LOGI(TAG, "%s ssid: %s, psk %s", __func__, config->ssid[i], config->psk[i]);
    }

    if (aos_kv_getint(WIFI_CONFIG_NUM_CUR_KEY, &config->cur_used_ssid) < 0) {
        config->cur_used_ssid = 0;
    }

}

static void wifi_config_add(app_wifi_config_t *config, char *ssid, char *psk)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    int i;

    for (i = 0; i < config->max_num; i ++) {
        if (strcmp(config->ssid[i], ssid) == 0 &&
            strcmp(config->psk[i], psk) == 0) {
                if ((config->empty_ssid_table & (1 << i))) {
                    config->empty_ssid_table &= ~(1 << i);
                }
                config->cur_used_ssid=i;
                LOGD(TAG, "wifi_config_add 1 cur_used_ssid %d", config->cur_used_ssid);
                aos_mutex_unlock(&wifi_config_mutex);
                return;
        }
    }

    strcpy(config->ssid[(config->cur_used_ssid + 1) % config->max_num], ssid);
    strcpy(config->psk[(config->cur_used_ssid + 1) % config->max_num], psk);

    config->cur_used_ssid = ((config->cur_used_ssid + 1) % config->max_num);
    config->empty_ssid_table &= ~(1 << config->cur_used_ssid);

    aos_mutex_unlock(&wifi_config_mutex);
}

static void wifi_config_del(app_wifi_config_t *config, char *ssid)
{
    int i;

    if (ssid == NULL) {
        return;
    }
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    for (i = 0; i < config->max_num; i ++) {
        if (!(config->empty_ssid_table & (1 << i)) &&
             strcmp(config->ssid[i], ssid) == 0) {
            config->empty_ssid_table |= (1 << i);
            aos_mutex_unlock(&wifi_config_mutex);
            return;
        }
    }
    aos_mutex_unlock(&wifi_config_mutex);
}

static void wifi_config_save(app_wifi_config_t *config)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    char wifi_ssid_key[32];
    char wifi_psk_key[32];
    char ssid[33] = {0};
    char psk[65] = {0};
    int i;
    int cur = 0;

    for (i = 0; i < config->max_num; i++) {
        snprintf(wifi_ssid_key, sizeof(wifi_ssid_key), WIFI_CONFIG_SSID_KEY_PATTERN, i);
        snprintf(wifi_psk_key, sizeof(wifi_psk_key), WIFI_CONFIG_PSK_KEY_PATTERN, i);

        if (config->empty_ssid_table & (1 << i)) {
            aos_kv_del(wifi_ssid_key);
            aos_kv_del(wifi_psk_key);
            continue;
        }

        aos_kv_getstring(wifi_ssid_key, ssid, sizeof(ssid));
        aos_kv_getstring(wifi_psk_key, psk, sizeof(psk));

        if (strcmp(ssid, config->ssid[i]) != 0 ||
            strcmp(psk, config->psk[i]) != 0) {
            
            LOGI(TAG, "%s ssid: %s, psk %s\n", __func__, config->ssid[i], config->psk[i]);
            aos_kv_setstring(wifi_ssid_key, config->ssid[i]);
            aos_kv_setstring(wifi_psk_key, config->psk[i]);
        }
    }

    int ret = aos_kv_getint(WIFI_CONFIG_NUM_CUR_KEY, &cur);

    if (ret < 0 || cur != config->cur_used_ssid) {
        aos_kv_setint(WIFI_CONFIG_NUM_CUR_KEY, config->cur_used_ssid);
    }
    aos_mutex_unlock(&wifi_config_mutex);
}

static void wifi_config_add_ap(app_wifi_config_t *config, char *ssid)
{
    if (config->ap_num >= WIFI_CONFIG_NUM_MAX_AP) {
        return;
    }
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    strcpy(config->ap_ssid[config->ap_num ++], ssid);
    aos_mutex_unlock(&wifi_config_mutex);
}

static int wifi_config_select_ssid(app_wifi_config_t *config, char **select_ssid, char **select_psk)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    int i, j;
    int old_ssid = config->cur_used_ssid;

    /* find the next saved ssid that is in the current ap list */
    for (j = 0; j < config->ap_num; j ++) {

        for (i = 0; i < config->max_num; i++) {
            config->cur_used_ssid = ((config->cur_used_ssid + 1) % config->max_num);
            if (old_ssid != config->cur_used_ssid && (!(config->empty_ssid_table & (1 << config->cur_used_ssid)) &&
                 (strcmp(config->ap_ssid[j], config->ssid[config->cur_used_ssid]) == 0) )) {
                *select_ssid = config->ssid[config->cur_used_ssid];
                *select_psk  = config->psk[config->cur_used_ssid];
                aos_mutex_unlock(&wifi_config_mutex);
                return config->cur_used_ssid;
            }
        }
    }

    /* if no qualified ssid, just pick the next saved ssid */
    for (i = 0; i < config->max_num; i++) {
        config->cur_used_ssid = ((config->cur_used_ssid + 1) % config->max_num);
        if (!(config->empty_ssid_table & (1 << config->cur_used_ssid))) {
            *select_ssid = config->ssid[config->cur_used_ssid];
            *select_psk  = config->psk[config->cur_used_ssid];
            aos_mutex_unlock(&wifi_config_mutex);
            return config->cur_used_ssid;
        }
    }
    aos_mutex_unlock(&wifi_config_mutex);
    return 0;
}

static int wifi_config_is_empty(app_wifi_config_t *config)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    int result;
    result = ((~config->empty_ssid_table) & ~(0xFFFFFFFF << config->max_num)) ? 0 : 1;
    aos_mutex_unlock(&wifi_config_mutex);
    return result;
}

static char *wifi_config_get_cur_ssid(app_wifi_config_t *config)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    if (!(config->empty_ssid_table & (1 << config->cur_used_ssid))) {
        aos_mutex_unlock(&wifi_config_mutex);
        return config->ssid[config->cur_used_ssid];
    }
    aos_mutex_unlock(&wifi_config_mutex);
    return NULL;
}

static int wifi_config_get_last_ssid_psk(app_wifi_config_t *config, char **select_ssid, char **select_psk)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);

    int i, j;
    for (j = 0; j < config->ap_num; j ++) {
        for (i = 0; i < config->max_num; i++) {
            if ((!(config->empty_ssid_table & (1 << config->cur_used_ssid)) &&
                 (strcmp(config->ap_ssid[j], config->ssid[config->cur_used_ssid]) == 0) )) {
                *select_ssid = config->ssid[config->cur_used_ssid];
                *select_psk  = config->psk[config->cur_used_ssid];
                aos_mutex_unlock(&wifi_config_mutex);
                return config->cur_used_ssid;
            }
            config->cur_used_ssid = ((config->cur_used_ssid + 1) % config->max_num);
        }
    }
        /* if no qualified ssid, just pick the next saved ssid */
    for (i = 0; i < config->max_num; i++) {
        config->cur_used_ssid = ((config->cur_used_ssid + 1) % config->max_num);
        if (!(config->empty_ssid_table & (1 << config->cur_used_ssid))) {
            *select_ssid = config->ssid[config->cur_used_ssid];
            *select_psk  = config->psk[config->cur_used_ssid];
            aos_mutex_unlock(&wifi_config_mutex);
            return config->cur_used_ssid;
        }
    }
    aos_mutex_unlock(&wifi_config_mutex);
    return -1;
}

static int wifi_config_get_ssid_num(app_wifi_config_t *config)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    int i, table, num = 0;

    table = config->empty_ssid_table;

    for (i = 0; i < config->max_num; i ++) {
        num += (!((table >> i) & 1));
    }
    aos_mutex_unlock(&wifi_config_mutex);
    return num;
}

static char *wifi_config_get_used_ssid(app_wifi_config_t *config)
{
    aos_mutex_lock(&wifi_config_mutex, AOS_WAIT_FOREVER);
    if (!(config->empty_ssid_table & (1 << config->cur_used_ssid))) {
        aos_mutex_unlock(&wifi_config_mutex);
        return config->ssid[config->cur_used_ssid];
    }
    aos_mutex_unlock(&wifi_config_mutex);
    return NULL;
}

void app_wifi_config_init()
{
    wifi_config_init(&g_wifi_config);
}

void app_wifi_config_save()
{
    wifi_config_save(&g_wifi_config);
}

void app_wifi_config_add(char *ssid, char *psk)
{
    wifi_config_add(&g_wifi_config, ssid, psk);
}

void app_wifi_config_del(char *ssid)
{
    wifi_config_del(&g_wifi_config, ssid);
}

int app_wifi_config_select_ssid(char **select_ssid, char **select_psk)
{
    return wifi_config_select_ssid(&g_wifi_config, select_ssid, select_psk);
}

int app_wifi_config_is_empty()
{
    return wifi_config_is_empty(&g_wifi_config);
}

int app_wifi_config_get_ssid_num()
{
    return wifi_config_get_ssid_num(&g_wifi_config);
}

void app_wifi_config_add_ap(char *ssid)
{
    wifi_config_add_ap(&g_wifi_config, ssid);
}

char* app_wifi_config_get_cur_ssid()
{
    return wifi_config_get_cur_ssid(&g_wifi_config);
}

char* app_wifi_config_get_used_ssid(void){
    return wifi_config_get_used_ssid(&g_wifi_config);
}

int app_wifi_config_get_last_ssid_psk(char **select_ssid, char **select_psk)
{
    return wifi_config_get_last_ssid_psk(&g_wifi_config, select_ssid, select_psk);
}