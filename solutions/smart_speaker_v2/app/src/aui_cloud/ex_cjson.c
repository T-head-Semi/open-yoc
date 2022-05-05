/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "ex_cjson.h"

cJSON *cJSON_GetObjectItemByPath(cJSON *object, const char *path)
{
    if (object == NULL) {
        return NULL;
    }

    char *dup_path = strdup(path);
    char *saveptr  = NULL;
    char *next     = strtok_r(dup_path, ".", &saveptr);

    cJSON *item = object;
    while (next) {
        char *arr_ptr = strchr(next, '[');
        if (arr_ptr) {
            *arr_ptr++ = '\0';
            item = cJSON_GetObjectItem(item, next);
            if (item) {
                item = cJSON_GetArrayItem(item, atoi(arr_ptr));
            }
        } else {
            item = cJSON_GetObjectItem(item, next);
        }
        if (item == NULL) {
            break;
        }

        next = strtok_r(NULL, ".", &saveptr);
    }

    free(dup_path);

    return item;
}

