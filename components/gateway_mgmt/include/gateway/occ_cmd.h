/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _APP_OCC_CMD_H_
#define _APP_OCC_CMD_H_

extern aos_sem_t js_sem;

int gateway_occ_getjs(char *pid);
void gateway_model_conv_init(void);
int gateway_occ_get_gw_triples(void);
int gateway_occ_get_dev_triples(uint8_t *pcid, uint8_t *pk, uint8_t *ps, uint8_t *dn, uint8_t *pid);
#endif

