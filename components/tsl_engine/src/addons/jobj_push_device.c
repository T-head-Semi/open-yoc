/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#include <tsl_engine/jobj_push_device.h>
#include <tsl_engine/stash_ref.h>
#include <tsl_engine/device_mesh.h>

#define TAG                    "addon_device"

static duk_ret_t _native_send_to_device(duk_context *ctx)
{
    size_t size;
    int rc = -1, i;
    device_t *dev;
    char *data = NULL;

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "hdl");
    if (!duk_is_pointer(ctx, -1)) {
        LOGE(TAG, "get inner hdl fail");
        goto err;
    }

    dev = (device_t*)duk_get_pointer(ctx, -1);

    duk_pop(ctx);

    if (!(duk_is_array(ctx, 0))) {
        LOGE(TAG, "parameter must be (array)");
        goto err;
    }

    size = duk_get_length(ctx, 0);

    data = aos_malloc(size);
    if (!data) {
        LOGE(TAG, "may be oom");
        goto err;
    }

    for (i = 0; i < size; i++) {
        duk_get_prop_index(ctx, 0, i);
        data[i] = duk_get_int(ctx, -1);
        duk_pop(ctx);
    }

    rc = dev_send_to_device(dev, (const uint8_t*)data, size);

err:
    duk_push_int(ctx, rc);
    aos_free(data);
    return 1;
}

static duk_ret_t _native_send_to_cloud(duk_context *ctx)
{
    int rc = -1;
    device_t *dev;
    char *data = NULL;

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "hdl");
    if (!duk_is_pointer(ctx, -1)) {
        LOGE(TAG, "get inner hdl fail");
        goto err;
    }

    dev = (device_t*)duk_get_pointer(ctx, -1);
    duk_pop(ctx);

    if (!(duk_is_string(ctx, 0))) {
        LOGE(TAG, "parameter must be json string");
        goto err;
    }

    data = (char *)duk_get_string(ctx, 0);

    rc = dev_send_to_cloud(dev, (const uint8_t*)data, strlen(data));

err:
    duk_push_int(ctx, rc);
    return 1;
}


static duk_ret_t _native_value_get(duk_context *ctx)
{
    int rc = -1;
    device_t *dev;
    const char *key;

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "hdl");
    if (!duk_is_pointer(ctx, -1)) {
        LOGE(TAG, "get inner hdl fail");
        goto err;
    }

    dev = (device_t*)duk_get_pointer(ctx, -1);

    duk_pop(ctx);

    if (!(duk_is_string(ctx, 0))) {
        LOGE(TAG, "parameter must be (num, str)");
        goto err;
    }

    key = duk_get_string(ctx, 0);
    rc  = (long)dev_kv_get(dev, key);

err:
    duk_push_int(ctx, rc);
    return 1;
}

static duk_ret_t _native_value_set(duk_context *ctx)
{
    long value;
    int rc = -1;
    device_t *dev;
    const char *key;

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "hdl");
    if (!duk_is_pointer(ctx, -1)) {
        LOGE(TAG, "get inner hdl fail");
        goto err;
    }

    dev = (device_t*)duk_get_pointer(ctx, -1);

    duk_pop(ctx);

    if (!(duk_is_string(ctx, 0) && duk_is_number(ctx, 1))) {
        LOGE(TAG, "parameter must be (num, str)");
        goto err;
    }

    key   = duk_get_string(ctx, 0);
    value = duk_get_int(ctx, 1);

    rc = dev_kv_set(dev, key, (void*)value);

err:
    duk_push_int(ctx, rc);
    return 1;
}

/**
 * @brief  push javascript obj for device
 * @param  [in] ctx
 * @param  [in] dev : handle of device
 * @return 0/-1
 */
int jobj_push_device(duk_context *ctx, device_t *dev)
{
    CHECK_PARAM(ctx && dev, -1);
    duk_push_object(ctx);

    duk_push_pointer(ctx, dev);
    duk_put_prop_string(ctx, -2, "hdl");
    duk_push_int(ctx, dev->type);
    duk_put_prop_string(ctx, -2, "type");
    duk_push_string(ctx, dev->name);
    duk_put_prop_string(ctx, -2, "name");

    duk_push_c_function(ctx, _native_send_to_device, 1);
    duk_put_prop_string(ctx, -2, "send_to_device");
    duk_push_c_function(ctx, _native_send_to_cloud, 1);
    duk_put_prop_string(ctx, -2, "send_to_cloud");

    duk_push_object(ctx);
    duk_push_pointer(ctx, dev);
    duk_put_prop_string(ctx, -2, "hdl");
    duk_push_c_function(ctx, _native_value_get, 1);
    duk_put_prop_string(ctx, -2, "get");
    duk_push_c_function(ctx, _native_value_set, 2);
    duk_put_prop_string(ctx, -2, "set");
    duk_put_prop_string(ctx, -2, "value");

    return 0;
}

/**
 * @brief  push javascript obj for dev_data
 * @param  [in] ctx
 * @param  [in] dev_data
 * @return 0/-1
 */
int jobj_push_dev_data(duk_context *ctx, dev_data_t *dev_data)
{
    CHECK_PARAM(ctx && dev_data, -1);
    duk_push_object(ctx);

    duk_push_pointer(ctx, dev_data);
    duk_put_prop_string(ctx, -2, "hdl");
    duk_push_int(ctx, dev_data->opcode);
    duk_put_prop_string(ctx, -2, "opcode");

    duk_idx_t arr_idx = duk_push_array(ctx);
    for (int i = 0; i < dev_data->data_size; i++) {
        duk_push_int(ctx, dev_data->data[i]);
        duk_put_prop_index(ctx, arr_idx, i);
    }
    duk_put_prop_string(ctx, -2, "data");

    return 0;
}

