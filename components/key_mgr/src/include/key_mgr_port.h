/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef KM_PORT_H
#define KM_PORT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

int get_data_from_addr(unsigned long addr, uint8_t *data, size_t data_len);

#endif
