/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdint.h>
#include <aos/kernel.h>

__attribute__((weak)) uint32_t msleep(uint32_t ms)
{
    if (ms) {
        aos_msleep(ms);
    }

    return 0;
}

