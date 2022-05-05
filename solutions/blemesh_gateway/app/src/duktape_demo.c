/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <aos/aos.h>
#include <aos/cli.h>
#include <vfs.h>
#include "littlefs_vfs.h"
#include <tsl_engine/duk_console.h>
#include "duktape_demo.h"
#include <tsl_engine/cloud_device_conv.h>
#include <tsl_engine/device.h>
#include <tsl_engine/jse.h>

#define TAG "duktape_demo"

#pragma GCC optimize ("O0")
static char* g_jsfile;
#if 0
int duktape_test1()
{
    duk_context *ctx = duk_create_heap_default();
    duk_eval_string(ctx, "1+2");
    printf("1+2=%d\n", (int) duk_get_int(ctx, -1));
    duk_destroy_heap(ctx);
    return 0;
}
#else

/* Being an embeddable engine, Duktape doesn't provide I/O
 * bindings by default.  Here's a simple one argument print()
 * function.
 */
static duk_ret_t native_print(duk_context *ctx)
{
    printf("%s\n", duk_to_string(ctx, 0));
    return 0;  /* no return value (= undefined) */
}

/* Adder: add argument values. */
static duk_ret_t native_adder(duk_context *ctx)
{
    int i;
    int n = duk_get_top(ctx);  /* #args */
    double res = 0.0;

    for (i = 0; i < n; i++) {
        res += duk_to_number(ctx, i);
    }

    duk_push_number(ctx, res);
    return 1;  /* one return value */
}

int duktape_test1()
{
    duk_context *ctx = duk_create_heap_default();

    duk_push_c_function(ctx, native_print, 1 /*nargs*/);
    duk_put_global_string(ctx, "print");
    duk_push_c_function(ctx, native_adder, DUK_VARARGS);
    duk_put_global_string(ctx, "adder");

    duk_eval_string_noresult(ctx, "print('2+3=' + adder(2, 3));");

    duk_destroy_heap(ctx);
    return 0;
}
#endif

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

/* Print error to stderr and pop error. */
static void print_pop_error(duk_context *ctx)
{
    printf("err:%s\n", duk_safe_to_stacktrace(ctx, -1));
    duk_pop(ctx);
}

static int handle_fh(duk_context *ctx, const char *path)
{
    struct stat st;
    char *buf = NULL;
    size_t bufsz;
    int rc, fd = -1, ret = -1;

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

    duk_push_pointer(ctx, (void *) buf);
    duk_push_uint(ctx, (duk_uint_t) bufsz);
    duk_push_string(ctx, path);

    rc = duk_safe_call(ctx, wrapped_compile_execute, NULL /*udata*/, 3 /*nargs*/, 1 /*nret*/);
    if (rc != DUK_EXEC_SUCCESS) {
        print_pop_error(ctx);
        goto err;
    } else {
        duk_pop(ctx);
    }

    ret = 0;
err:
    aos_freep(&buf);
    aos_close(fd);
    return ret;
}

#if 0
static duk_ret_t string_frombufferraw(duk_context *ctx)
{
    duk_buffer_to_string(ctx, 0);
    return 1;
}
#endif

static void _duktask(void *arg)
{
    int rc = -1;
    duk_context *ctx = duk_create_heap_default();

    if (ctx) {
        duk_push_c_function(ctx, native_print, 1 /*nargs*/);
        duk_put_global_string(ctx, "print");
        duk_console_init(ctx, 0);

#if 0
        duk_eval_string(ctx,
                        "(function(v){"
                        "if (typeof String === 'undefined') { String = {}; }"
                        "Object.defineProperty(String, 'fromBufferRaw', {value:v, configurable:true});"
                        "})");
        duk_push_c_function(ctx, string_frombufferraw, 1 /*nargs*/);
        (void) duk_pcall(ctx, 1);
        duk_pop(ctx);
#endif

        rc = handle_fh(ctx, g_jsfile);
        duk_destroy_heap(ctx);
    }
    LOGD(TAG, "rc = %8d, js file = %s", rc, g_jsfile);

    aos_freep(&g_jsfile);
    return;
}

int duktape_test2()
{
    struct stat st;
    char *buf = NULL;
    size_t bufsz;
    duk_context *ctx = NULL;
    int rc, fd = -1, ret = -1;
    char *path = "/lfs/test2.js";

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

    ctx = duk_create_heap_default();
    if (ctx) {
        duk_push_c_function(ctx, native_print, 1 /*nargs*/);
        duk_put_global_string(ctx, "print");
        duk_console_init(ctx, 0);

        // compile source code
        duk_compile_lstring(ctx, 0, buf, st.st_size); // compiles to an ECMAScript function and puts it on the stack top
        // since this is a global script, it must be executed before calling the invidual functions
        duk_call(ctx,0); // perhaps consider using duk_pcall() instead
        //  Step 2 (b):
        // push the global object on the stack to get its properties
        duk_push_global_object(ctx);

        // obtain the function with the name "funcA" and push it on the stack
        duk_get_prop_string(ctx, -1, "funcA"); // key name / global function name

        duk_require_function(ctx,-1); // require this is a function before calling it

        // now invoke it!
        // duk_push_* arguments here
        duk_call(ctx,0); // 0: number of arguments

        // duk_get_* (ctx,-1); to obtain return value

        // pop function return value
        duk_pop(ctx);

        // current stack top: global object

        // get next function
        duk_get_prop_string(ctx, -1, "funcB");

        // require it's a function
        duk_require_function(ctx, -1);

        duk_push_int(ctx, 2);
        duk_push_int(ctx, 3);
        // invoke it!
        duk_call(ctx, 2);  /* [ ... func 2 3 ] -> [ 5 ] */
        printf("2+3=%d\n", duk_get_int(ctx, -1));

        // pop return and global object off stack
        duk_pop_2(ctx);
    }

    ret = 0;
err:
    aos_freep(&buf);
    aos_close(fd);
    duk_destroy_heap(ctx);
    return ret;
}

static duk_context *_ctx;
duk_context *js_get_context()
{
    if (!_ctx) {
        _ctx = duk_create_heap_default();

        duk_push_c_function(_ctx, native_print, 1 /*nargs*/);
        duk_put_global_string(_ctx, "print");
        duk_console_init(_ctx, 0);
    }

    return _ctx;
}

int duktape_test3()
{
    duk_context *ctx = NULL;
    int rc, fd = -1, ret = -1;

    ctx = js_get_context();
    if (ctx) {
        duk_push_object(ctx);
        //module_device_register(ctx);
        duk_put_global_string(ctx, "__native");

        //duk_eval_string(ctx, "DEVICE.open(\"aaa\")");
        duk_eval_string(ctx, "__native.DEVICE.open(\"aaa\")");
        printf("2+3=%p\n", duk_get_pointer(ctx, -1));

        // pop function return value
        duk_pop(ctx);
    }

    ret = 0;
err:
    duk_destroy_heap(ctx);
    _ctx = NULL;
    return ret;
}


static void duk_help()
{
    printf("\tduk js_file[/lfs/xx.js]\n");
    printf("\tduk test1\n");
    printf("\tduk test2\n");
    printf("\tduk test3\n");
    printf("\tduk require\n");
    printf("\tduk require1\n");
    printf("\tduk cloud_to_device\n");
    printf("\tduk help");
}

static void _lwapi_init()
{
    static int init;

    if (!init) {
        extern int device_register_mesh();
        device_register_mesh();
    }
}

static void duktape_require1(void *arg)
{
    int osize = 0;
    _lwapi_init();
    jse_init();
    jse_start();

    return;
}

static void cmd_duk_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 2) {
        if (strcmp(argv[1], "help") == 0) {
            duk_help();
        } else if (strcmp(argv[1], "test1") == 0) {
            duktape_test1();
        } else if (strcmp(argv[1], "test2") == 0) {
            duktape_test2();
        } else if (strcmp(argv[1], "test3") == 0) {
            //duktape_test3();
        } else if (strcmp(argv[1], "require") == 0) {
            //extern int duktape_require();
            //duktape_require();
        } else if (strcmp(argv[1], "require1") == 0) {
            aos_task_new("duk_req1", duktape_require1, NULL, 20*1024);
        } else if (strcmp(argv[1], "cloud_to_device") == 0) {
            int rc;
            dev_conf_t conf;
            const char *cloud_data = "{\"powerstate\":1}";

            device_t *dev = dev_new(DEV_TYPE_MESH, "dev1");
            dev_config(dev, (const dev_conf_t*)&conf);

            rc = cloud_to_device(dev, cloud_data, strlen(cloud_data));
            printf("rc = %d\n", rc);
            dev_free(dev);
        } else if (strcmp(argv[1], "device_to_cloud") == 0) {
            int rc;
            dev_conf_t conf;
            const char dev_data[] = {0x82, 0x02, 0x01, 0x01};

            device_t *dev = dev_new(DEV_TYPE_MESH, "dev1");
            dev_config(dev, (const dev_conf_t*)&conf);

            rc = device_to_cloud(dev, dev_data, sizeof(dev_data));
            printf("rc = %d\n", rc);
            dev_free(dev);
        } else if (strcmp(argv[1], "compile") == 0) {
			size_t size;
			char *buf = get_file_content(TLS_CONVERT_SCRIPT_PATH, &size);
            printf("jcode = %p, jsize = %u\n", buf, size);
			char *bcode = jse_bytecode_compile(buf, size, &size);
            printf("bcode = %p, bsize = %u\n", bcode, size);
			aos_free(buf);
			aos_free(bcode);
		} else {
            g_jsfile = strdup(argv[1]);
            LOGD(TAG, "g_jsfile = %s", g_jsfile);
            aos_task_new("duk_task", _duktask, NULL, 10*1024);
        }
    } else {
        duk_help();
    }
}

int cli_reg_cmd_duk(void)
{
    static const struct cli_command cmd_info = {
        "duk",
        "duk example",
        cmd_duk_func,
    };

    aos_cli_register_command(&cmd_info);

    return 0;
}



