#ifndef __GATEWAY_MAIN_H__
#define __GATEWAY_MAIN_H__

//#include <iotx_log.h>
#include "ulog/ulog.h"

#define gateway_debug(...) LOGD("gateway", __VA_ARGS__)
#define gateway_info(...) LOGI("gateway", __VA_ARGS__)
#define gateway_warn(...) LOGW("gateway", __VA_ARGS__)
#define gateway_err(...) LOGE("gateway", __VA_ARGS__)
#define gateway_crit(...) LOGE("gateway", __VA_ARGS__)

#define GATEWAY_YIELD_THREAD_NAME "linkkit_yield"
#define GATEWAY_YIELD_THREAD_STACKSIZE (8 * 1024)
#define GATEWAY_YIELD_TIMEOUT_MS (1000)
#define GATEWAY_OTA_BUFFER_LEN (512 + 1)

//#define GATEWAY_SUPPORT_TOPO_CHANGE

typedef struct
{
    int master_devid;
    int cloud_connected;
    int master_initialized;
    int permit_join;
    char permit_join_pk[PRODUCT_KEY_MAXLEN];
    void *g_user_dispatch_thread;
    int g_user_dispatch_thread_running;
    int started;
} gateway_ctx_t;

extern int gateway_main(void *paras);
extern gateway_ctx_t *gateway_get_ctx(void);

#endif
