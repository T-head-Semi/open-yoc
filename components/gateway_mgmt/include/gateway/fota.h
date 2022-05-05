/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _GATEWAY_FOTA_H_
#define _GATEWAY_FOTA_H_

typedef struct {
    uint8_t type;
    uint8_t val[6];
} device_mac_t;

void gateway_fota_start(void);
void gateway_fota_rpt_gw_ver(void);
int  gateway_fota_is_downloading(void);
void gateway_fota_do_check(void);
void gateway_fota_set_auto_check(int enable);

int gateway_fota_init(void);
int gateway_fota_device_load();
int gateway_fota_device_add(const char *cid, const char *version, const device_mac_t *mac);
int gateway_fota_device_delete(const char *cid);

#endif

