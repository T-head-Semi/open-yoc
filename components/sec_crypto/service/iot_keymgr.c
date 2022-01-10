/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <keymgr.h>


int sc_get_bootkey(uint8_t *bk, int bk_size)
{

    return 0;
}

int sc_verify_bootkey(uint8_t *bk, int bk_size, uint8_t *bk_hash)
{

    return 0;
}


int sc_srv_get_auth_key(uint8_t *salt, uint32_t salt_len, uint8_t *key, uint32_t *len)
{
    return key_derivation(salt, salt_len, key, len);
}
