/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#include <vfs.h>
#include <tsl_engine/jse.h>
#include <tsl_engine/jni_buildin.h>
#include <tsl_engine/stash_ref.h>
#include <tsl_engine/duk_console.h>
#include <tsl_engine/duk_module_node.h>
#include "k_api.h"

#define TAG "jse"

typedef struct {
    char            *jcode;
    size_t          jcode_size;
    char            *bcode;
    size_t          bsize;
    int             ret;
} bc_param_t;

static struct {
#define MESSAGE_NUM (20)
    duk_context    *ctx;
    aos_queue_t    queue;
    uint8_t        qbuf[sizeof(jse_msg_t) * MESSAGE_NUM];
} _jse;

#if CONFIG_JSE_STATIC_STACK
static ktask_t          g_jse_task;
static cpu_stack_t      g_jse_task_stack[CONFIG_JSE_TASK_STACK_SIZE / sizeof(cpu_stack_t)];
#endif

static duk_ret_t cb_resolve_module(duk_context *ctx)
{
    const char *module_id;
    const char *parent_id;

    module_id = duk_require_string(ctx, 0);
    parent_id = duk_require_string(ctx, 1);

    //duk_push_sprintf(ctx, "%s.js", module_id);
    duk_push_sprintf(ctx, "%s", module_id);
    printf("resolve_cb: id:'%s', parent-id:'%s', resolve-to:'%s'\n",
           module_id, parent_id, duk_get_string(ctx, -1));

    return 1;
}

static duk_ret_t cb_load_module(duk_context *ctx)
{
    const char *filename;
    const char *module_id;
#if defined (AOS_COMP_VFS) && AOS_COMP_VFS
    struct aos_stat st;
#else
    struct stat st;
#endif        
    char *buf = NULL;
    size_t bufsz;
    int rc, fd = -1;
    char path[64];
    //char *path = "/lfs/hello.js";

    module_id = duk_require_string(ctx, 0);
    duk_get_prop_string(ctx, 2, "filename");
    filename = duk_require_string(ctx, -1);

    snprintf(path, sizeof(path), "/lfs/%s", filename + 2);
    printf("load_cb: id:'%s', filename:'%s', path = %s\n", module_id, filename, path);
    rc = aos_stat(path, &st);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    fd = aos_open(path, O_RDONLY);
    CHECK_RET_TAG_WITH_GOTO(fd > 0, err);

    bufsz = st.st_size;
    buf   = (char *) aos_zalloc(bufsz);
    if (!buf) {
        goto err;
    }
    rc = aos_read(fd, buf, st.st_size);
    CHECK_RET_TAG_WITH_GOTO(rc == st.st_size, err);

#if 1
    duk_push_lstring(ctx, buf, st.st_size);
err:
    aos_freep(&buf);
    aos_close(fd);
#else
    if (strcmp(module_id, "pig.js") == 0) {
        duk_push_sprintf(ctx, "module.exports = 'you\\'re about to get eaten by %s';",
                         module_id);
    } else if (strcmp(module_id, "cow.js") == 0) {
        duk_push_string(ctx, "module.exports = require('pig');");
    } else if (strcmp(module_id, "ape.js") == 0) {
        duk_push_string(ctx, "module.exports = { module: module, __filename: __filename, wasLoaded: module.loaded };");
    } else if (strcmp(module_id, "badger.js") == 0) {
        duk_push_string(ctx, "exports.foo = 123; exports.bar = 234;");
    } else if (strcmp(module_id, "comment.js") == 0) {
        duk_push_string(ctx, "exports.foo = 123; exports.bar = 234; // comment");
    } else if (strcmp(module_id, "shebang.js") == 0) {
        duk_push_string(ctx, "#!ignored\nexports.foo = 123; exports.bar = 234;");
    } else {
        (void) duk_type_error(ctx, "cannot find module: %s", module_id);
    }
#endif

    return 1;
}

static duk_ret_t handle_print(duk_context *ctx)
{
    printf("%s\n", duk_safe_to_string(ctx, 0));
    return 0;
}

static duk_ret_t handle_assert(duk_context *ctx)
{
    if (duk_to_boolean(ctx, 0)) {
        return 0;
    }
    (void) duk_generic_error(ctx, "assertion failed: %s", duk_safe_to_string(ctx, 1));
    return 0;
}

/* Print error to stderr and pop error. */
static void print_pop_error(duk_context *ctx)
{
    printf("err:%s\n", duk_safe_to_stacktrace(ctx, -1));
    duk_pop(ctx);
}

static duk_ret_t wrapped_compile_execute(duk_context *ctx, void *udata)
{
    const char *src_data;
    duk_size_t src_len;
    duk_uint_t comp_flags;
    (void) udata;

    src_data = (const char *) duk_require_pointer(ctx, -3);
    src_len = (duk_size_t) duk_require_uint(ctx, -2);

    /* Source code. */
    comp_flags = DUK_COMPILE_SHEBANG;
    duk_compile_lstring_filename(ctx, comp_flags, src_data, src_len);

    duk_push_global_object(ctx);  /* 'this' binding */
    duk_call_method(ctx, 0);

    return 0;  /* duk_safe_call() cleans up */
}

/**
 * @brief  init the js-engine
 * @return 0/-1
 */
int jse_init()
{
    duk_context *ctx = NULL;

    if (!_jse.ctx) {
        aos_queue_new(&_jse.queue, _jse.qbuf, sizeof(jse_msg_t) * MESSAGE_NUM, sizeof(jse_msg_t));
        ctx = duk_create_heap_default();
        if (!ctx) {
            LOGE(TAG, "jse ctx create fail");
            goto err;
        }
        stash_ref_setup(ctx);

        duk_console_init(ctx, 0);
        duk_push_c_function(ctx, handle_print, 1);
        duk_put_global_string(ctx, "print");
        duk_push_c_function(ctx, handle_assert, 2);
        duk_put_global_string(ctx, "assert");

        duk_push_object(ctx);
        duk_push_c_function(ctx, cb_resolve_module, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "resolve");
        duk_push_c_function(ctx, cb_load_module, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "load");
        duk_module_node_init(ctx);

        _jse.ctx = ctx;
        return 0;
    } else {
        LOGI(TAG, "jse init already");
        return 0;
    }

err:
    duk_destroy_heap(ctx);
    aos_queue_free(&_jse.queue);
    return -1;
}

static int _jse_start_app(duk_context *ctx, const char *path)
{
    int rc;
    char *buf;
    size_t size;

    buf = get_file_content(path, &size);
    if (!buf) {
        LOGE(TAG, "get app file fail");
        return -1;
    }

    duk_push_pointer(ctx, (void*)buf);
    duk_push_uint(ctx, (duk_uint_t)size);
    duk_push_string(ctx, path);

    rc = duk_safe_call(ctx, wrapped_compile_execute, NULL /*udata*/, 3 /*nargs*/, 1 /*nret*/);
    if (rc != DUK_EXEC_SUCCESS) {
        print_pop_error(ctx);
        rc = -1;
        goto err;
    }
    duk_pop(ctx);
    rc = 0;

err:
    aos_free(buf);
    return rc;
}

static void _jse_task(void *arg)
{
    size_t len;
    jse_msg_t msg;
    duk_context *ctx = _jse.ctx;

    for (;;) {
        aos_queue_recv(&_jse.queue, AOS_WAIT_FOREVER, &msg, &len);
        if (msg.callback && len == sizeof(jse_msg_t)) {
            msg.callback(ctx, msg.param);
            if (aos_sem_is_valid(&msg.sem)) {
                aos_sem_signal(&msg.sem);
            }
        }
    }
}


/**
 * @brief  start the jse-loop-task
 * @return 0/-1
 */
int jse_start()
{
    int rc;
#if CONFIG_JSE_STATIC_STACK
    rc = krhino_task_create(&g_jse_task, "jse_task", NULL,
                            AOS_DEFAULT_APP_PRI - 2, 0u, g_jse_task_stack,
                            CONFIG_JSE_TASK_STACK_SIZE/sizeof(cpu_stack_t), _jse_task, 1u);
#else
    rc = aos_task_new_ext(&jse_task, "jse_task", _jse_task, NULL, CONFIG_JSE_TASK_STACK_SIZE, AOS_DEFAULT_APP_PRI - 2);
    if (rc != 0) {
        LOGE(TAG, "jse task start fail");
    }
#endif
    return rc;
}

static int _jse_msg_send(jse_callback_t call, void *arg, int sync)
{
    jse_msg_t msg;
    int rc, cnt = 0;

    CHECK_PARAM(call, -1);
    memset(&msg, 0, sizeof(msg));
    if (sync)
        aos_sem_new(&msg.sem, 0);
    msg.callback = call;
    msg.param    = arg;
    //FIXME:
    for (;;) {
        rc = aos_queue_send(&_jse.queue, &msg, sizeof(jse_msg_t));
        if (rc == 0) {
            break;
        } else {
            if (cnt++ % 10 == 0)
                printf("jse send fail, queue may be full, cnt = %d, rc = %d", cnt, rc);
            aos_msleep(20);
        }
    }
    if (sync) {
        aos_sem_wait(&msg.sem, AOS_WAIT_FOREVER);
        aos_sem_free(&msg.sem);
    }

    return 0;
}

static void _cb_bytecode_compile(void *jse, void *arg)
{
    void *bc_ptr;
    size_t bc_len = 0;
    duk_context *ctx  = jse;
    bc_param_t *param = arg;

    duk_compile_lstring(ctx, 0, (const char*)param->jcode, param->jcode_size);
    duk_dup_top(ctx);
    duk_dump_function(ctx);
    bc_ptr = duk_require_buffer_data(ctx, -1, &bc_len);
    if (bc_ptr && bc_len) {
        param->bcode = aos_malloc(bc_len);
        if (!param->bcode) {
            LOGE(TAG, "may be oom");
            goto err;
        }
        memcpy(param->bcode, bc_ptr, bc_len);
        param->bsize = bc_len;
        param->ret   = 0;
    }

err:
    duk_pop_2(ctx);
    duk_gc(ctx, 0);
}

/**
 * @brief  compile javascript code original to bytecode
 * @param  [in] jcode  javascript code
 * @param  [in] jsize  size of the jcode
 * @param  [in] osize  bytecode size
 * attention: return value should be free by the caller
 * @return NULL on error
 */
char* jse_bytecode_compile(const char *jcode, size_t jsize, size_t *osize)
{
    int rc = -1;
    char *bcode = NULL;
    bc_param_t *param = NULL;

    CHECK_PARAM(jcode && jsize && osize, NULL);
    param = aos_zalloc(sizeof(bc_param_t));
    param->ret        = -1;
    param->jcode      = (char*)jcode;
    param->jcode_size = jsize;

    rc = jse_msg_send_sync(_cb_bytecode_compile, (void*)param);
    if (!(rc == 0 && param->ret == 0)) {
        rc = -1;
        LOGE(TAG, "msg send fail");
        goto err;
    }
    *osize = param->bsize;
    bcode  = param->bcode;

err:
    aos_freep((char**)&param);

    return bcode;
}

/**
 * @brief  send message to the jse-task and excute it async
 * @param  [in] call callback will be called by jse-task
 * @param  [in] arg args for callback
 * @return 0/-1
 */
int jse_msg_send(jse_callback_t call, void *arg)
{
    return _jse_msg_send(call, arg, 0);
}

/**
 * @brief  send message to the jse-task and excute it
 * @param  [in] call callback will be called by jse-task
 * @param  [in] arg args for callback
 * @return 0/-1
 */
int jse_msg_send_sync(jse_callback_t call, void *arg)
{
    return _jse_msg_send(call, arg, 1);
}


