/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <aos/hal/rng.h>
#include <aos/kernel.h>
#include <errno.h>
#include <drv/rng.h>

int32_t hal_random_num_read(random_dev_t random, void *buf, int32_t bytes)
{
    if (buf == NULL) {
        return -1;
    }
    return csi_rng_get_multi_word(buf, bytes);
}
