/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int key_derivation(uint8_t *salt, uint32_t salt_len, void *key_out, uint32_t *key_olen);