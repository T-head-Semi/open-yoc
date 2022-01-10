/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#ifndef _SC_KEYMGR_H_
#define _SC_KEYMGR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int sc_get_bootkey(uint8_t *bk, int bk_size);
int sc_verify_bootkey(uint8_t *bk, int bk_size, uint8_t *bk_hash);
int sc_srv_get_auth_key(uint8_t *key, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif