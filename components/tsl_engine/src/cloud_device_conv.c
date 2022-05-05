/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#include <tsl_engine/jse.h>
#include <tsl_engine/jobj_push_device.h>
#include <tsl_engine/cloud_device_conv.h>
#include <tsl_engine/file_utils.h>

#define TAG "conv"

typedef struct {
    char            *jcode;
    size_t          jcode_size;
    device_t        *dev;
    char            *idata;
    size_t          isize;
    //char            *odata;
    //size_t          osize;
    int             ret;
} conv_param_t;

static void _cb_cloud_to_device(void *jse, void *arg)
{
    duk_context *ctx    = jse;
    conv_param_t *param = arg;

#if CONFIG_TSL_BYTECODE_ENABLE
    {
        void *buf = duk_push_fixed_buffer(ctx, param->jcode_size);
        memcpy(buf, (const void *)param->jcode, param->jcode_size);
        duk_load_function(ctx);
    }
#else
    // compiles to an ECMAScript function and puts it on the stack top
    duk_compile_lstring(ctx, 0, param->jcode, param->jcode_size);
#endif

    // since this is a global script, it must be executed before calling the invidual functions
    duk_call(ctx,0); // perhaps consider using duk_pcall() instead
    //  Step 2 (b):
    // push the global object on the stack to get its properties
    duk_push_global_object(ctx);

    // obtain the function with the name "funcA" and push it on the stack
    duk_get_prop_string(ctx, -1, "runner"); // key name / global function name
    duk_get_prop_string(ctx, -1, CLOUD_TO_DEVICE_METHOD); // key name / global function name

    jobj_push_device(ctx, param->dev);
    duk_push_string(ctx, param->idata);
    // invoke it!
    if (duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
        printf("%s", duk_safe_to_stacktrace(ctx, -1));
    } else {
        param->ret = duk_get_int(ctx, -1);
    }

    // pop return and global object off stack
    duk_pop_2(ctx);

    duk_gc(ctx, 0);
}

/**
 * @brief  convert tsl from cloud to device
 * @param  [in] dev
 * @param  [in] in     input data
 * @param  [in] isize  input size of in data
 * @param  [in] osize  output size
 * @return 0/-1
 */
int cloud_to_device(device_t *dev, const char *in, size_t isize)
{
    int rc = -1;
    size_t size;
    char *buf = NULL;
    conv_param_t *param = NULL;

    CHECK_PARAM(dev && in && isize, -1);
    param = aos_zalloc(sizeof(conv_param_t));
#if CONFIG_SAVE_JS_TO_RAM
    buf   = get_js_from_ram(dev->name, &size);
#else
    buf   = get_file_content(TLS_CONVERT_SCRIPT_PATH, &size);
#endif
    if (!(buf && param)) {
        LOGE(TAG, "get file fail or oom");
        goto err;
    }

    param->ret        = -1;
    param->dev        = dev;
    param->jcode      = buf;
    param->jcode_size = size;
    param->idata      = (char*)in;
    param->isize      = isize;
    rc = jse_msg_send_sync(_cb_cloud_to_device, (void*)param);
    if (!(rc == 0 && param->ret == 0)) {
        rc = -1;
        LOGE(TAG, "msg send fail");
        goto err;
    }

err:
    aos_freep((char**)&param);
#if !CONFIG_SAVE_JS_TO_RAM
    aos_freep((char**)&buf);
#endif

    return rc;
}

static void _cb_device_to_cloud(void *jse, void *arg)
{
    duk_context *ctx    = jse;
    conv_param_t *param = arg;

#if CONFIG_TSL_BYTECODE_ENABLE
    {
        void *buf = duk_push_fixed_buffer(ctx, param->jcode_size);
        memcpy(buf, (const void *)param->jcode, param->jcode_size);
        duk_load_function(ctx);
    }
#else
    // compiles to an ECMAScript function and puts it on the stack top
    duk_compile_lstring(ctx, 0, param->jcode, param->jcode_size);
#endif

    // since this is a global script, it must be executed before calling the invidual functions
    duk_call(ctx,0); // perhaps consider using duk_pcall() instead
    //  Step 2 (b):
    // push the global object on the stack to get its properties
    duk_push_global_object(ctx);

    // obtain the function with the name "funcA" and push it on the stack
    duk_get_prop_string(ctx, -1, "runner"); // key name / global function name
    duk_get_prop_string(ctx, -1, DEVICE_CLOUD_TO_METHOD); // key name / global function name

    jobj_push_device(ctx, param->dev);
    jobj_push_dev_data(ctx, (dev_data_t *)param->idata);

    // invoke it!
    if (duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
        printf("%s", duk_safe_to_stacktrace(ctx, -1));
    } else {
        param->ret = duk_get_int(ctx, -1);
    }

    // pop return and global object off stack
    duk_pop_2(ctx);

    duk_gc(ctx, 0);
}

/**
 * @brief  convert tsl from device to cloud
 * @param  [in] dev
 * @param  [in] in     input data
 * @param  [in] isize  input size of in data
 * @param  [in] osize  output size
 * @return 0/-1
 */
int device_to_cloud(device_t *dev, const char *in, size_t isize)
{
    int rc = -1;
    size_t size;
    char *buf = NULL;
    conv_param_t *param = NULL;

    CHECK_PARAM(dev && in && isize, -1);
    param = aos_zalloc(sizeof(conv_param_t));
#if CONFIG_SAVE_JS_TO_RAM
    buf   = get_js_from_ram(dev->name, &size);
#else
    buf   = get_file_content(TLS_CONVERT_SCRIPT_PATH, &size);
#endif
    if (!(buf && param)) {
        LOGE(TAG, "get file fail or oom");
        goto err;
    }

    param->ret        = -1;
    param->dev        = dev;
    param->jcode      = buf;
    param->jcode_size = size;
    param->idata      = (char*)in;
    param->isize      = isize;
    rc = jse_msg_send_sync(_cb_device_to_cloud, (void*)param);
    if (!(rc == 0 && param->ret == 0)) {
        rc = -1;
        LOGE(TAG, "msg send fail");
        goto err;
    }

err:
    aos_freep((char**)&param);
#if !CONFIG_SAVE_JS_TO_RAM
    aos_freep((char**)&buf);
#endif

    return rc;
}


