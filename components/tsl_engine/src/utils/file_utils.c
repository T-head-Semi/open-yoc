/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#include <vfs.h>
#include <tsl_engine/file_utils.h>
#include <tsl_engine/jse.h>

#define TAG "file"

#if defined (CONFIG_SAVE_JS_TO_RAM) && (CONFIG_SAVE_JS_TO_RAM == 1)
#define JS_SIZE_MAX 20*1024
#define JS_NAME_SIZE 32
#define JS_ITEM_MAX 10

typedef struct _js_item_s {
    char valid;
    char js_name[JS_NAME_SIZE+1];
    char *js_buf;
    int js_size;
} js_item_t;

typedef struct _js_content_s {
    js_item_t js_item[JS_ITEM_MAX];
} js_content_t;

js_content_t *js_content = NULL;

int init_js_to_ram(void)
{
    js_content = aos_zalloc(sizeof(js_content_t));
    if (js_content == NULL) {
        LOGE(TAG, "alloc js buf fail");
        return -1;
    }

    return 0;
}

int save_js_to_ram(const char *js_name, const char *data, size_t size)
{
    int i;
    size_t js_size = 0;
    char *js_data = NULL;

    if (size > JS_SIZE_MAX) {
        LOGE(TAG, "js size is too large");
        return -1;
    }

    for (i = 0; i < JS_ITEM_MAX; i++) {
        if (js_content->js_item[i].valid == 0) {
            js_content->js_item[i].valid = 1;
            break;
        }
    }

    if (i == JS_ITEM_MAX) {
        LOGE(TAG, "js content full");
        return -1;
    }
    
    memcpy(js_content->js_item[i].js_name, js_name, strlen(js_name)>JS_NAME_SIZE?JS_NAME_SIZE:strlen(js_name));

#if CONFIG_TSL_BYTECODE_ENABLE    
    js_data = jse_bytecode_compile(data, size, &js_size);
    LOGD(TAG, "bcode %p, bcode_size %d", js_data, js_size);
#else
    js_size = size;
    js_data = data;
#endif

    js_content->js_item[i].js_buf = aos_zalloc(js_size);
    if (js_content->js_item[i].js_buf == NULL) {
        LOGE(TAG, "js_buf alloc fail");
        return -1;
    }

    memcpy(js_content->js_item[i].js_buf, js_data, js_size);
    js_content->js_item[i].js_size = js_size;

#if CONFIG_TSL_BYTECODE_ENABLE
    aos_free(js_data);
#endif
    return 0;
}

char *get_js_from_ram(const char *js_name, size_t *osize)
{
    for (int i = 0; i < JS_ITEM_MAX; i++) {
        if (strcmp(js_content->js_item[i].js_name, js_name) == 0) {
            *osize = js_content->js_item[i].js_size;
            return js_content->js_item[i].js_buf;
        }
    }

    return NULL;
}

int is_js_already_existed(const char *js_name)
{
    for (int i = 0; i < JS_ITEM_MAX; i++) {
        if ((js_content->js_item[i].valid == 1) && (strcmp(js_content->js_item[i].js_name, js_name) == 0)) {
            return 1;
        }
    }

    return 0;
}

#endif

char *get_file_content(const char *path, size_t *osize)
{
#if defined (AOS_COMP_VFS) && AOS_COMP_VFS
    struct aos_stat st;
#else
    struct stat st;
#endif
    char *buf = NULL;
    int rc, fd = -1;

    CHECK_PARAM(path && osize, NULL);
    memset(&st, 0, sizeof(st));
    aos_stat(path, &st);
    fd = aos_open(path, O_RDONLY);
    if (!(st.st_size && fd > 0)) {
        LOGE(TAG, "open fail, size = %u, fd = %d", st.st_size, fd);
        goto err;
    }

    buf = (char *) aos_zalloc(st.st_size);
    if (!buf) {
        LOGE(TAG, "may be oom, size = %d", st.st_size);
        goto err;
    }
    rc = aos_read(fd, buf, st.st_size);
    if (rc != st.st_size) {
        LOGE(TAG, "read fail, size = %u, rc = %d", st.st_size, rc);
        goto err;
    }

    *osize = st.st_size;
    aos_close(fd);
    return buf;

err:
    aos_freep(&buf);
    if (fd > 0)
        aos_close(fd);
    return NULL;
}


