/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __EX_CJSON_H__
#define __EX_CJSON_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C"
{
#endif

cJSON *cJSON_GetObjectItemByPath(cJSON *object, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* __EX_CJSON_H__ */

