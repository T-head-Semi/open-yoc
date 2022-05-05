/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#ifndef _WIFI_CONFIG_H_
#define _WIFI_CONFIG_H_

/* for multiple ssid/psk management */
void app_wifi_config_init(void);
void app_wifi_config_save(void);
void app_wifi_config_add(char *ssid, char *psk);
void app_wifi_config_del(char *ssid);
int app_wifi_config_select_ssid(char **select_ssid, char **select_psk);
int app_wifi_config_is_empty(void);
int app_wifi_config_get_ssid_num(void);
void app_wifi_config_add_ap(char *ssid);
char* app_wifi_config_get_cur_ssid(void);
char* app_wifi_config_get_used_ssid(void);
int app_wifi_config_get_last_ssid_psk(char **select_ssid, char **select_psk);

#endif
