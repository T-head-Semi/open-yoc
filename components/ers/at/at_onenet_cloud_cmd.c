/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <aos/kernel.h>
#include <uservice/event.h>
#include "at_internal.h"

#include <cis_api.h>
#include "cis_internals.h"


#ifdef CONFIG_YOC_LPM
#include <yoc/lpm.h>
#endif

#define TAG "onenet_at"


/*******************************************************************************
       Macro definition
******************************************************************************/
#define S(x) #x
#define STR(x) S(x)

#define STR_VALUE_LEN         161
#define OPAQUE_VALUE_LEN      161
#define ON_WRITE_LEN          70
#define ONENET_STACK_SIZE     2048
#define MAX_ONE_NET_AT_SIZE   120
#define MAX_SET_PARAM_SIZE    80
#define MAX_SET_PARAM_AT_SIZE 150
#define MAX_INS_MAP_SIZE      30
#define CIS_PARAM_ERROR       601
#define CIS_STATUS_ERROR      602
#define CIS_UNKNOWN_ERROR     100

#define AT_RSP_OK           "\r\nOK\r\n"

struct onenet_conf {
    int total_size;
    char *cur_config;
    int index;
    int cur_size;
    int flag;
};

struct onenet_addobj {
    unsigned int ref;
    unsigned int obj_id;
    unsigned int ins_count;
    char ins_map[MAX_INS_MAP_SIZE];
    unsigned int attr_count;
    unsigned int action_count;
};

struct onenet_update {
    unsigned int ref;
    unsigned int lifetime;
    unsigned int with_obj_flag;
};

struct onenet_discover_rsp {
    unsigned int ref;
    unsigned int msg_id;
    unsigned int result;
    unsigned int length;
    char         value[MAX_SET_PARAM_SIZE];
};

struct result_code_map {
    unsigned char at_result_code;
    unsigned char coap_result_code;
};

struct onenet_parameter_rsp {
    unsigned int ref;
    unsigned int msg_id;
    unsigned int result;
};

struct onenet_delobj {
    unsigned int ref;
    unsigned int obj_id;
};

struct onenet_write_exe {
    unsigned int ref;
    unsigned int msg_id;
    unsigned int result;
};

struct onenet_notify_read {
    unsigned int ref;
    unsigned int msg_id;
    unsigned int result;
    unsigned int obj_id;
    unsigned int ins_id;
    unsigned int res_id;
    unsigned int value_type;
    unsigned int len;
    char *value;
    unsigned int index;
    unsigned int flag;
    unsigned int ackid;
};

enum onenet_rsp_type {
    RSP_READ = 0,
    RSP_WRITE,
    RSP_EXECUTE,
    RSP_OBSERVE,
    RSP_SETPARAMS,
    RSP_DISCOVER,
};

//183.230.40.40, no bootstrap
static uint8_t config_hex[] = {0x13, 0x00, 0x34,
                               0xf1, 0x00, 0x03,
                               0xf2, 0x00, 0x21, 0x05, 0x00/*mtu*/,
                               0x11/*Link&bind type*/,
                               0x00, 0x00/*vpn*/,
                               0x00, 0x00/*username*/,
                               0x00, 0x00/*password*/,
                               0x00, 0x0d/*host length*/,
                               0x31, 0x38, 0x33, 0x2e, 0x32, 0x33, 0x30, 0x2e, 0x34, 0x30,
                               0x2e, 0x34, 0x30, 0x00, 0x04, 0x4e, 0x55, 0x4c, 0x4c, 0xf3,
                               0x00, 0x0d, 0x00, 0xea, 0x04, 0x00,
                               0x00, 0x04, 0x4e, 0x55, 0x4c, 0x4c
                              };

static void *onenet_context = NULL;

static cis_time_t g_lifetime = 300;

static bool g_shutdown = true;

static aos_task_t g_at_oneopen_hdl = { NULL };

static aos_sem_t client_opensem;

static int onet_deinit(void)
{
    cis_ret_t ret;

    if (onenet_context) {
        ret = cis_deinit(&onenet_context);

        if (ret != CIS_RET_OK) {
            LOGE(TAG, "deinit %d", ret);
            return ret;
        }

        onenet_context = NULL;
    }

    return 0;
}

static uint8_t get_coap_result_code(uint8_t  at_result_code)
{
    unsigned int index = 0;
    static const struct result_code_map code_map[] = {
        { 1,  CIS_COAP_205_CONTENT },
        { 2,  CIS_COAP_204_CHANGED},
        { 11, CIS_COAP_400_BAD_REQUEST },
        { 12, CIS_COAP_401_UNAUTHORIZED },
        { 13, CIS_COAP_404_NOT_FOUND },
        { 14, CIS_COAP_405_METHOD_NOT_ALLOWED },
        { 15, CIS_COAP_406_NOT_ACCEPTABLE }
    };

    for (; index < sizeof(code_map) / sizeof(struct result_code_map); index++) {
        if (code_map[index].at_result_code == at_result_code) {
            return code_map[index].coap_result_code;
        }
    }

    return CIS_COAP_503_SERVICE_UNAVAILABLE;
}

static int check_coap_result_code(int code, enum onenet_rsp_type type)
{
    int res = -1;

    switch (type) {
        case RSP_READ:
        case RSP_OBSERVE:
        case RSP_DISCOVER:
            if (code == 1 || code == 11 || code == 12 ||
                code == 13 || code == 14 || code == 15) {
                res = 0;
            }

            break;

        case RSP_WRITE:
        case RSP_EXECUTE:
        case RSP_SETPARAMS:
            if (code == 2 || code == 11 || code == 12
                || code == 13 || code == 14) {
                res = 0;
            }

            break;

        default:
            break;
    }

    return res;
}

static int onet_conf_init()
{
    int ret;
    uint8_t bs_flag = 0;

    if (bs_flag) { //need bootstrap
        config_hex[31] = 0x33;
        config_hex[32] = 0x39;
        config_hex[42] = 0x01;
    } else {
        config_hex[31] = 0x34;
        config_hex[32] = 0x30;
        config_hex[42] = 0x00;
    }

    ret = cis_init(&onenet_context, config_hex, sizeof(config_hex));

    if (ret != CIS_RET_OK) {
        LOGE(TAG, "cis init ret=%d", ret);
        return -1;
    }

    return 0;
}

void at_cmd_onet_miplcreate(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        if (onenet_context != NULL) {
            LOGE(TAG, "onenet_context");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (onet_conf_init() != 0) {

            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
#ifdef CONFIG_YOC_LPM
            LOGD(TAG, "onenet agree sleep");
            aos_lpm_agree_sleep(LPM_PROC_CLOUD_ONENET);
#endif
            return;
        }

#ifdef CONFIG_YOC_LPM
        LOGD(TAG, "onenet join run");
        aos_lpm_join_run(LPM_PROC_CLOUD_ONENET);
#endif
        int ret = aos_sem_new(&client_opensem, 0);

        if (ret != AT_OK) {
            LOGE(TAG, "sem");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }
        atserver_send("\r\n%s:%d\r\nOK\r\n", "+MIPLCREATE", 1);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

static int onet_have_context(unsigned int ref)
{
    //onenet only support single ref
    if (ref != 1 || onenet_context == NULL) {
        return -1;
    }

    return 0;
}

void at_cmd_onet_mipldel(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        unsigned int ref = 0;

        if ((NULL == data) || (0 == strlen(data))) {
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        data[strlen(data)] = '\0';

        if (!is_digit_str((char *)data)) {
            LOGE(TAG, "argv err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        ref = atoi((char *)data);
        LOGD(TAG, "mipdel ref=%d", ref);

        if (onet_have_context(ref) != 0) {
            LOGE(TAG, "onet_have_context");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (!g_shutdown) {
            LOGE(TAG, "open task running");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (onet_deinit()) {
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

#ifdef CONFIG_YOC_LPM
        LOGD(TAG, "onenet agree sleep");
        aos_lpm_agree_sleep(LPM_PROC_CLOUD_ONENET);
#endif
        aos_sem_free(&client_opensem);
        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

static cis_ret_t onet_mipladdobj_req(struct onenet_addobj *para)
{
    cis_ret_t ret = 0;
    cis_inst_bitmap_t bitmap = {0};
    uint8_t *instPtr;
    uint16_t instBytes;
    cis_res_count_t  rescount;
    int i;

    /*make bitmap */
    bitmap.instanceCount = para->ins_count;
    instBytes = (bitmap.instanceCount - 1) / 8 + 1;
    instPtr = (uint8_t *)aos_malloc(instBytes);

    if (instPtr == NULL) {
        return CIS_RET_MEMORY_ERR;
    }

    memset(instPtr, 0, instBytes);

    for (i = 0; i < bitmap.instanceCount; i++) {
        uint8_t instBytePos = i / 8;
        uint8_t instByteOffset = 7 - (i % 8);

        if (*(para->ins_map + i) == '1') {
            instPtr[instBytePos] += 0x01 << instByteOffset;
        }
    }

    bitmap.instanceBitmap = instPtr;

    bitmap.instanceBytes = (para->ins_count - 1) / 8 + 1;
    //bitmap.attr_count = para->attr_count;

    rescount.attrCount = para->attr_count;
    rescount.actCount = para->action_count;

    ret = cis_addobject(onenet_context, para->obj_id, &bitmap, &rescount);

    aos_free(instPtr);

    return ret;
}

void at_cmd_onet_mipladdobj(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        struct onenet_addobj param = {0};
        void *p[]  = {&param.ref, &param.obj_id, &param.ins_count,
                      param.ins_map, &param.attr_count, &param.action_count
                     };

        /* parse param */
        ret = at_parse_param_safe("%d,%d,%d,%" STR(MAX_INS_MAP_SIZE) "s,%d,%d", (char *)data, p);

        if (ret < 0 || ret != (sizeof(p) / sizeof(void *))) {
            LOGE(TAG, "parse");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (param.ins_count != strlen(param.ins_map)) {
            LOGE(TAG, "param");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        ret = onet_have_context(param.ref);

        if (ret != CIS_RET_NO_ERROR) {
            LOGE(TAG, "onet_have_context %d", ret);
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        ret = onet_mipladdobj_req(&param);

        if (ret != CIS_RET_NO_ERROR) {
            LOGE(TAG, "onet_mipladdobj_req %d", ret);
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_mipldelobj(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        struct onenet_delobj param = {0};
        char *argv[2];
        int ret;
        ret = strsplit(argv, 2, data, ",");

        if (ret != 2) {
            LOGE(TAG, "param len");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (!is_digit_str(argv[0])) {
            LOGE(TAG, "ref err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.ref = atoi(argv[0]);

        if (!is_digit_str(argv[1])) {
            LOGE(TAG, "obj_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.obj_id = atoi(argv[1]);

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "onet_have_context");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        ret = cis_delobject(onenet_context, param.obj_id);

        if (ret != CIS_RET_NO_ERROR) {
            LOGE(TAG, "cis_delobject=%d", ret);
            AT_BACK_CIS_ERR(ret);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

static void onet_at_pump(void *param)
{
//    cis_time_t g_lifetime_last = 0;
    //g_lifetime_last = cissys_gettime();
    g_shutdown = false;

    while (!g_shutdown) {
        if (onenet_context != NULL) {
            uint32_t pumpRet;
            int timeout = g_lifetime;
            /*pump function*/
            pumpRet = cis_pump(onenet_context, &timeout);

            LOGD(TAG, "cis_pump %d", timeout);

            if (pumpRet == PUMP_RET_NOSLEEP) {
                timeout = 1;
            }

            packet_read(onenet_context, timeout);

#if 0

            if ((!cisnet_attached_state(onenet_context))
                && (nowtime - g_lifetime_last > g_lifetime - 10))) {
                if (cis_update_reg(onenet_context, LIFETIME_INVALID, false) == COAP_NO_ERROR) {
                    g_lifetime_last = utils_gettime_s();
                    //g_auto_update = true;
                }
            }

#endif
        }

        aos_msleep(100);
    }

    aos_sem_signal(&client_opensem);
    aos_task_exit(0);
}

static cis_coapret_t onet_on_read_cb(void *context, cis_uri_t *uri, cis_mid_t mid)
{
    char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

    if (at_str == NULL) {
        return CIS_RESPONSE_SERVICE_UNAVAILABLE;
    }

    if (!CIS_URI_IS_SET_RESOURCE(uri)) {
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLREAD: %d,%d,%d,%d,%d\r\n",
                 1, mid, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1, -1);
    } else {
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLREAD: %d,%d,%d,%d,%d\r\n",
                 1, mid, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1, uri->resourceId);
    }

    AT_BACK_STR(at_str);
    aos_free(at_str);

    return CIS_CALLBACK_CONFORM;
}

static void get_write_flag(int i, int maxi, int *flag)
{
    if (i == 0) {
        *flag = 0;
    } else if (i == maxi) {
        *flag = 1;
    } else {
        *flag = 2;
    }

}

//attention: onenet server only support cis_data_type_opaque
static cis_coapret_t onet_on_write_cb(void *context, cis_uri_t *uri, const cis_data_t *value,
                                      cis_attrcount_t attrcount, cis_mid_t mid)
{
    int i;

    if (!CIS_URI_IS_SET_INSTANCE(uri)) {
        return CIS_CALLBACK_BAD_REQUEST;
    }

    for (i = attrcount - 1; i >= 0; i--) {
        cis_data_t *data = (cis_data_t *)value + (attrcount - 1 - i);
        int flag = 0;
        get_write_flag(i, attrcount - 1, &flag);

        switch (data->type) {
            case cis_data_type_opaque: {
                char *at_str = NULL;
                char *strBuffer = NULL;
                int str_len = ON_WRITE_LEN + data->asBuffer.length * 2 + 1;
                strBuffer = aos_malloc(data->asBuffer.length * 2 + 1);

                if (strBuffer == NULL) {
                    return CIS_CALLBACK_SERVICE_UNAVAILABLE;
                }

                at_str = aos_malloc(str_len);

                if (at_str == NULL) {
                    aos_free(strBuffer);
                    return CIS_CALLBACK_SERVICE_UNAVAILABLE;
                }

                memset(strBuffer, 0, data->asBuffer.length * 2 + 1);

                //at_bytes2hexstr(strBuffer, data->asBuffer.buffer, data->asBuffer.length);
                bytes2hexstr(strBuffer, (data->asBuffer.length * 2 + 1), data->asBuffer.buffer, data->asBuffer.length);

                snprintf(at_str, str_len, "\r\n+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%s,%d,%d\r\n",
                         1, mid, uri->objectId, uri->instanceId, data->id, data->type,
                         data->asBuffer.length, strBuffer, flag, i);

                AT_BACK_STR(at_str);
                aos_free(strBuffer);
                aos_free(at_str);
                break;
            }

            case cis_data_type_string: {
                char *at_str = NULL;
                int str_len = ON_WRITE_LEN + data->asBuffer.length;
                at_str = aos_malloc(str_len);

                if (at_str == NULL) {
                    return CIS_CALLBACK_SERVICE_UNAVAILABLE;
                }

                snprintf(at_str, str_len, "\r\n+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%s,%d,%d\r\n",
                         1, mid, uri->objectId, uri->instanceId, data->id, data->type,
                         data->asBuffer.length, data->asBuffer.buffer, flag, i);

                AT_BACK_STR(at_str);
                aos_free(at_str);
                break;
            }

            case cis_data_type_integer: {
                char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

                if (at_str == NULL) {
                    return CIS_CALLBACK_SERVICE_UNAVAILABLE;
                }

                snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", 1,
                         mid, uri->objectId, uri->instanceId, data->id, data->type,
                         4, (int)data->value.asInteger, flag, i);

                AT_BACK_STR(at_str);
                aos_free(at_str);
                break;
            }

            case cis_data_type_float: {
                char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

                if (at_str == NULL) {
                    return CIS_CALLBACK_SERVICE_UNAVAILABLE;
                }

                snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%.3f,%d,%d\r\n",
                         1, mid, uri->objectId, uri->instanceId, data->id, data->type,
                         8, data->value.asFloat, flag, i);
                AT_BACK_STR(at_str);

                aos_free(at_str);
                break;
            }

            case cis_data_type_bool: {
                char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

                if (at_str == NULL) {
                    return CIS_CALLBACK_SERVICE_UNAVAILABLE;
                }

                snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                         1, mid, uri->objectId, uri->instanceId, data->id, data->type,
                         1, data->value.asBoolean, flag, i);
                AT_BACK_STR(at_str);

                aos_free(at_str);
                break;
            }

            default:
                return CIS_CALLBACK_METHOD_NOT_ALLOWED;
        }
    }

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onet_on_execute_cb(void *context, cis_uri_t *uri, const uint8_t *value,
                                        uint32_t length, cis_mid_t mid)
{
    char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

    if (at_str == NULL) {
        return CIS_CALLBACK_SERVICE_UNAVAILABLE;
    }

    memset(at_str, 0, MAX_ONE_NET_AT_SIZE);

    if (length > 0) {
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEXECUTE: %d,%d,%d,%d,%d,%d,",
                 1, mid, uri->objectId, uri->instanceId, uri->resourceId, length);
        int tmp = MAX_ONE_NET_AT_SIZE - strlen(at_str) - 4;
        memcpy(at_str + strlen(at_str), value, length < tmp ? length : tmp);
        strcat(at_str, "\r\n");
    } else {
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEXECUTE: %d,%d,%d,%d,%d\r\n",
                 1, mid, uri->objectId, uri->instanceId, uri->resourceId);
    }

    AT_BACK_STR(at_str);
    aos_free(at_str);

    return CIS_CALLBACK_CONFORM;
}

static cis_ret_t onet_on_observe_cb(void *context, cis_uri_t *uri, bool flag, cis_mid_t mid)
{
    char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

    if (at_str == NULL) {
        return CIS_CALLBACK_SERVICE_UNAVAILABLE;
    }

    snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLOBSERVE: %d,%d,%d,%d,%d,%d\r\n",
             1, mid, flag, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1, CIS_URI_IS_SET_RESOURCE(uri) ? uri->resourceId : -1);

    AT_BACK_STR(at_str);
    aos_free(at_str);

    return CIS_CALLBACK_CONFORM;
}

static cis_coapret_t onet_on_discover_cb(void *context, cis_uri_t *uri, cis_mid_t mid)
{
    char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

    if (at_str == NULL) {
        return CIS_CALLBACK_SERVICE_UNAVAILABLE;
    }

    snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLDISCOVER: %d,%d,%d\r\n", 1, mid, uri->objectId); //len parameter

    AT_BACK_STR(at_str);
    aos_free(at_str);
    return CIS_CALLBACK_CONFORM;
}

static cis_ret_t onet_on_parameter_cb(void *context, cis_uri_t *uri, cis_observe_attr_t parameters, cis_mid_t mid)
{
    char *at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

    if (at_str == NULL) {
        return CIS_CALLBACK_SERVICE_UNAVAILABLE;
    }

    char *str_params = aos_malloc(MAX_SET_PARAM_SIZE);

    if (str_params == NULL) {
        aos_free(at_str);
        return CIS_CALLBACK_SERVICE_UNAVAILABLE;
    }

    cis_iid_t ins_id = CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1;
    cis_rid_t res_id = CIS_URI_IS_SET_RESOURCE(uri) ? uri->resourceId : -1;

    memset(str_params, 0, MAX_SET_PARAM_SIZE);

    if ((parameters.toSet & ATTR_FLAG_MIN_PERIOD) != 0) {
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "pmin=%d",  parameters.minPeriod);
    }

    if ((parameters.toSet & ATTR_FLAG_MAX_PERIOD) != 0) {
        if (strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }

        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "pmax=%d",  parameters.maxPeriod);
    }

    if ((parameters.toSet & ATTR_FLAG_LESS_THAN) != 0) {
        if (strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }

        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "lt=%.1f",  parameters.lessThan);
    }

    if ((parameters.toSet & ATTR_FLAG_GREATER_THAN) != 0) {
        if (strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }

        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "gt=%.1f",  parameters.greaterThan);
    }

    if ((parameters.toSet & ATTR_FLAG_STEP) != 0) {
        if (strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }

        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "stp=%.1f",  parameters.step);
    }

    LOGD(TAG, "[onet_on_parameter_cb]:%s", str_params);
    snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLPARAMETER: %d,%d,%d,%d,%d,%d,\"%s\"\r\n", 1, mid, uri->objectId, ins_id, res_id, strlen(str_params), str_params);

    AT_BACK_STR(at_str);
    aos_free(at_str);
    aos_free(str_params);
    return CIS_CALLBACK_CONFORM;
}

static void onet_on_event_cb(void *context, cis_evt_t eid, void *param)
{
    char *at_str = NULL;
    //static uint8_t s_auto_update = false;

    if (eid == CIS_EVENT_UPDATE_NEED) {
        cis_update_reg(onenet_context, LIFETIME_INVALID, false);
        //s_auto_update = true;
    }

#if 0

    if (eid == CIS_EVENT_UPDATE_SUCCESS || eid == CIS_EVENT_UPDATE_FAILED
        || eid == CIS_EVENT_UPDATE_TIMEOUT) {
        st_server_t *targetP = ((st_context_t *)context)->server;
        g_lifetime_last = targetP->registration;

        if (s_auto_update) {
            s_auto_update = false;
            return;
        }
    }

#endif

    at_str = aos_malloc(MAX_ONE_NET_AT_SIZE);

    if (at_str == NULL) {
        return;
    }

    snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEVENT: %d,%d", 1, eid);

    switch (eid) {
        case CIS_EVENT_NOTIFY_SUCCESS:
        case CIS_EVENT_NOTIFY_FAILED:
        case CIS_EVENT_UPDATE_NEED:
        case CIS_EVENT_RESPONSE_FAILED: {
            snprintf(at_str + strlen(at_str), MAX_ONE_NET_AT_SIZE - strlen(at_str), ",%d", (int)param);
            break;
        }

        default: {
            break;
        }
    }

    snprintf(at_str + strlen(at_str), MAX_ONE_NET_AT_SIZE - strlen(at_str), "\r\n");

    AT_BACK_STR(at_str);
    aos_free(at_str);
}

static int onet_client_open(unsigned int liftime)
{
    if (g_at_oneopen_hdl.hdl == NULL) {
        cis_callback_t callback;
        callback.onRead = (cis_read_callback_t)onet_on_read_cb;
        callback.onWrite = (cis_write_callback_t)onet_on_write_cb;
        callback.onExec = (cis_exec_callback_t)onet_on_execute_cb;
        callback.onObserve = (cis_observe_callback_t)onet_on_observe_cb;
        callback.onSetParams = (cis_set_params_callback_t)onet_on_parameter_cb;
        callback.onEvent = (cis_event_callback_t)onet_on_event_cb;
        callback.onDiscover = (cis_discover_callback_t)onet_on_discover_cb;

        if (cis_register(onenet_context, liftime, &callback) != CIS_RET_OK) {
            return -1;
        }

        return aos_task_new_ext(&g_at_oneopen_hdl, "at_onenet", onet_at_pump,
                                NULL, ONENET_STACK_SIZE, AOS_DEFAULT_APP_PRI);
    } else {
        return -1;
    }
}

void at_cmd_onet_miplopen(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        unsigned int ref = 0;
        unsigned int timeout = 0;
        void *param[]  = { &ref, &g_lifetime, &timeout};

        /* parse param */
        ret = at_parse_param_safe("%d,%d,%d", (char *)data, param);

        if (ret < 0 || ret != (sizeof(param) / sizeof(void *))) {
            LOGE(TAG, "parse ret: %x", ret);
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (onet_have_context(ref) != 0) {
            LOGE(TAG, "context");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (onet_client_open(g_lifetime)) {
            LOGE(TAG, "open");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_miplclose(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        unsigned int ref = 0;

        //data[len] = '\0';

        if (!is_digit_str((char *)data)) {
            LOGE(TAG, "ref err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        ref = atoi((char *)data);
        LOGD(TAG, "miplclose ref=%d", ref);

        if (onet_have_context(ref) != 0) {
            LOGE(TAG, "onet_have_context");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (g_at_oneopen_hdl.hdl != NULL) {

            /* we shoud ensure that we do cis_unregister succeed */
            while (cis_isregister_enabled(onenet_context)) {
                cis_unregister(onenet_context);
                cisnet_break_recv(NULL, NULL);
                aos_msleep(100);
            }

            g_shutdown = true;
            aos_sem_wait(&client_opensem, -1);
            g_at_oneopen_hdl.hdl = NULL;
            AT_BACK_OK();
            return;
        }

        AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

static int onet_valuetype_check(int len, cis_datatype_t type)
{
    if (type == cis_data_type_integer) {
        if (len != 2 && len != 4 && len != 8) {
            return -1;
        }
    } else if (type == cis_data_type_float) {
        if (len != 4 && len != 8) {
            return -1;
        }
    } else if (type == cis_data_type_bool) {
        if (len != 1) {
            return -1;
        }
    } else if (type == cis_data_type_string) {
        if (len >= STR_VALUE_LEN) {
            return -1;
        }
    } else if (type == cis_data_type_opaque) {
        if ((len * 2) >= OPAQUE_VALUE_LEN) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

static cis_ret_t onet_read_param(struct onenet_notify_read *param, cis_data_t *dataP)
{
    dataP->type = param->value_type;
    dataP->id = param->res_id;
    char *endprt = NULL;

    if (dataP->type == cis_data_type_integer) {
        dataP->value.asInteger = strtol(param->value, &endprt, 10);
        if (*endprt != '\0') {
            return CIS_RET_ERROR;
        }
    } else if (dataP->type == cis_data_type_float) {
        dataP->value.asFloat = strtof((char *)param->value, &endprt);
        if (*endprt != '\0') {
            return CIS_RET_ERROR;
        }
    } else if (dataP->type == cis_data_type_bool) {
        int value = strtol((char *)param->value, &endprt, 10);

        if (*endprt != '\0' || (value != 0 && value != 1)) {
            return CIS_RET_ERROR;
        }
        dataP->value.asBoolean = value;
    } else if (dataP->type == cis_data_type_string) {
        dataP->asBuffer.length = param->len;
        dataP->asBuffer.buffer = (uint8_t *)(param->value);
    } else if (dataP->type == cis_data_type_opaque) {
        dataP->asBuffer.length = param->len;
        dataP->asBuffer.buffer = (uint8_t *)(param->value);
    } else {
        return CIS_CALLBACK_NOT_FOUND;
    }

    return CIS_RET_OK;
}


static cis_ret_t onet_notify_data(struct onenet_notify_read *param)
{
    cis_data_t tmpdata = {0};
    cis_uri_t uri = {0};
    cis_ret_t ret = 0;
    cis_coapret_t result = CIS_NOTIFY_CONTINUE;

    if ((param->flag == 0 && param->index != 0) || (param->flag != 0 && param->index == 0)) {
        return CIS_RET_PARAMETER_ERR;
    }

    if (param->ackid != 0 && param->ackid != 1) {
        return CIS_RET_PARAMETER_ERR;
    }

    uri.objectId = param->obj_id;
    uri.instanceId = param->ins_id;
    uri.resourceId = param->res_id;
    cis_uri_update(&uri);

    if ((ret = onet_read_param(param, &tmpdata)) != CIS_RET_OK) {
        return ret;
    }

    if (param->flag == 0 && param->index == 0) {
        result = CIS_NOTIFY_CONTENT;
    }

    ret = cis_notify(onenet_context, &uri, &tmpdata, param->msg_id, result, param->ackid);

    return ret;
}

static cis_ret_t onet_miplnotify_req(struct onenet_notify_read *param)
{
    cis_ret_t ret = CIS_RET_INVILID;

    st_object_t *targetP = (st_object_t *)CIS_LIST_FIND(((st_context_t *)onenet_context)->objectList, param->obj_id);

    if (targetP == NULL) {
        return CIS_RET_PARAMETER_ERR;
    }

    ret = onet_notify_data(param);

    return ret;
}

void at_cmd_onet_miplnotify(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        struct onenet_notify_read param = {0};
        char *argv[10];
        int ret;
        ret = strsplit(argv, 10, data, ",");

        if (ret != 10) {
            LOGE(TAG, "param len");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (!is_digit_str(argv[0])) {
            LOGE(TAG, "ref err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.ref = atoi(argv[0]);

        if (!is_digit_str(argv[1])) {
            LOGE(TAG, "msg_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.msg_id = atoi(argv[1]);

        if (!is_digit_str(argv[2])) {
            LOGE(TAG, "obj_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.obj_id = atoi(argv[2]);

        if (!is_digit_str(argv[3])) {
            LOGE(TAG, "ins_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.ins_id = atoi(argv[3]);

        if (!is_digit_str(argv[4])) {
            LOGE(TAG, "res_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.res_id = atoi(argv[4]);

        if (!is_digit_str(argv[5])) {
            LOGE(TAG, "msg_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.value_type = atoi(argv[5]);

        if (!is_digit_str(argv[6])) {
            LOGE(TAG, "len err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.len = atoi(argv[6]);

        if (!is_digit_str(argv[8])) {
            LOGE(TAG, "index err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.index = atoi(argv[8]);

        if (!is_digit_str(argv[9])) {
            LOGE(TAG, "flag err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.flag = atoi(argv[9]);

        if (onet_valuetype_check(param.len, param.value_type) != 0) {
            LOGE(TAG, "type");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.value = aos_malloc(STR_VALUE_LEN);

        if (param.value == NULL) {
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        memcpy(param.value, argv[7], strlen(argv[7]) + 1);

        if (param.value_type == cis_data_type_string && param.len != strlen(param.value)) {
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "onet_have_context");
            aos_free(param.value);
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        ret = onet_miplnotify_req(&param);

        if (ret != CIS_RET_OK && ret != COAP_205_CONTENT) {
            AT_BACK_CIS_ERR(ret);
            aos_free(param.value);
            return;
        }

        AT_BACK_OK();
        aos_free(param.value);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

static cis_ret_t onet_read_data(struct onenet_notify_read *param)
{
    cis_data_t tmpdata = {0};
    cis_uri_t uri = {0};
    cis_ret_t ret = 0;
    static uint32_t flag = 0;
    static uint32_t index = 0;

    if ((param->flag == 0 && param->index != 0) || (param->flag != 0 && param->index == 0)) {
        flag = 0;
        index = 0;
        return CIS_RET_PARAMETER_ERR;
    }

    if (param->flag == 1) {
        if (flag == 0) {
            index = param->index;
            flag = 1;
        } else {
            //flag = 0;
            //index = 0;
            return CIS_RET_PARAMETER_ERR;
        }
    } else if (param->flag == 2) {
        if (flag == 1 || flag == 2) {
            if (index - 1 == param->index) {
                index = param->index;
            } else {
                //flag = 0;
                //index = 0;
                return CIS_RET_PARAMETER_ERR;
            }
        } else {
            return CIS_RET_PARAMETER_ERR;
        }
    } else if (param->flag == 0) {
        flag = 0;
        index = 0;
    } else {
        flag = 0;
        index = 0;
        return CIS_RET_PARAMETER_ERR;
    }

    uri.objectId = param->obj_id;
    uri.instanceId = param->ins_id;
    uri.resourceId = param->res_id;
    cis_uri_update(&uri);

    if ((ret = onet_read_param(param, &tmpdata)) != CIS_RET_OK) {
        return ret;
    }

    ret = cis_response(onenet_context, &uri, &tmpdata, param->msg_id, CIS_RESPONSE_CONTINUE);

    if (param->flag == 0 && param->index == 0) {
        ret = cis_response(onenet_context, NULL, NULL, param->msg_id, CIS_RESPONSE_READ);
        //return ret;
    }

    return ret;
}

static cis_ret_t onet_miplread_req(struct onenet_notify_read *param)
{
    cis_ret_t ret = CIS_RET_INVILID;

    st_object_t *targetP = (st_object_t *)CIS_LIST_FIND(((st_context_t *)onenet_context)->objectList, param->obj_id);

    if (targetP == NULL) {
        return CIS_RET_PARAMETER_ERR;
    }

    ret = onet_read_data(param);

    return ret;
}

void at_cmd_onet_miplreadrsp(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        struct onenet_notify_read param = {0};
        int ret;
        cis_coapret_t result;
        param.value = aos_malloc(STR_VALUE_LEN);

        if (param.value == NULL) {
            LOGE(TAG, "malloc");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        void *params[]  = {&param.ref, &param.msg_id, &param.result,
                           &param.obj_id, &param.ins_id, &param.res_id,
                           &param.value_type, &param.len, param.value, &param.index, &param.flag
                          };

        /* parse param */
        ret = at_parse_param_safe("%d,%d,%d,%d,%d,%d,%d,%d,%" STR(STR_VALUE_LEN) "s,%d,%d",
                                  (char *)data, params);

        if (ret < 0 || ret != (sizeof(params) / sizeof(void *))) {
            LOGE(TAG, "parse");
            aos_free(param.value);
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (onet_valuetype_check(param.len, param.value_type) != 0) {
            LOGE(TAG, "type");
            aos_free(param.value);
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if ((param.value_type == cis_data_type_string) && (param.len != strlen(param.value))) {
            LOGE(TAG, "len");
            aos_free(param.value);
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "onet_have_context");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            aos_free(param.value);
            return;
        }



        if (0 != check_coap_result_code(param.result, RSP_READ)) {
            LOGE(TAG, "result");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if ((result = get_coap_result_code(param.result)) != CIS_COAP_205_CONTENT) {
            cis_response(onenet_context, NULL, NULL, param.msg_id, result);
            aos_free(param.value);
            AT_BACK_OK();
            return;
        }

        LOGD(TAG, "msgid=%d,objid=%d", param.msg_id, param.obj_id);
        ret = onet_miplread_req(&param);

        if (ret != CIS_RET_OK) {
            LOGE(TAG, "onet_miplread_req ret=%d!", ret);
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            aos_free(param.value);
            return;
        }

        AT_BACK_OK();
        aos_free(param.value);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_miplwritersp(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        struct onenet_write_exe param = {0};
        int ret;
        char *argv[3];
        ret = strsplit(argv, 3, data, ",");

        if (ret != 3) {
            LOGE(TAG, "param len");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (!is_digit_str(argv[0])) {
            LOGE(TAG, "ref err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.ref = atoi(argv[0]);

        if (!is_digit_str(argv[1])) {
            LOGE(TAG, "msg_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.msg_id = atoi(argv[1]);

        if (!is_digit_str(argv[2])) {
            LOGE(TAG, "result err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.result = atoi(argv[2]);

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "onet_have_context err");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (0 != check_coap_result_code(param.result, RSP_WRITE)) {
            LOGE(TAG, "result");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        ret = cis_response(onenet_context, NULL, NULL, param.msg_id,
                           get_coap_result_code(param.result));

        if (ret != COAP_NO_ERROR) {
            LOGE(TAG, "cis_notify err");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_miplexecutersp(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        struct onenet_write_exe param = {0};
        int ret;
        char *argv[3];
        ret = strsplit(argv, 3, data, ",");

        if (ret != 3) {
            LOGE(TAG, "param len");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (!is_digit_str(argv[0])) {
            LOGE(TAG, "ref err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.ref = atoi(argv[0]);

        if (!is_digit_str(argv[1])) {
            LOGE(TAG, "msg_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.msg_id = atoi(argv[1]);

        if (!is_digit_str(argv[2])) {
            LOGE(TAG, "result err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.result = atoi(argv[2]);

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "onet_have_context err");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (0 != check_coap_result_code(param.result, RSP_EXECUTE)) {
            LOGE(TAG, "result");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        ret = cis_response(onenet_context, NULL, NULL, param.msg_id,
                           get_coap_result_code(param.result));

        if (ret != COAP_NO_ERROR) {
            LOGE(TAG, "cis_notify err");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_miplobserveresp(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        struct onenet_write_exe param = {0};
        int ret;
        char *argv[3];
        ret = strsplit(argv, 3, data, ",");

        if (ret != 3) {
            LOGE(TAG, "param len");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (!is_digit_str(argv[0])) {
            LOGE(TAG, "ref err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.ref = atoi(argv[0]);

        if (!is_digit_str(argv[1])) {
            LOGE(TAG, "msg_id err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.msg_id = atoi(argv[1]);

        if (!is_digit_str(argv[2])) {
            LOGE(TAG, "result err!");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        param.result = atoi(argv[2]);

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "onet_have_context err");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (0 != check_coap_result_code(param.result, RSP_OBSERVE)) {
            LOGE(TAG, "result");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        ret = cis_response(onenet_context, NULL, NULL, param.msg_id,
                           get_coap_result_code(param.result));

        if (ret != CIS_RET_OK) {
            LOGE(TAG, "cis_notify err");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_miplparameterresp(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        struct onenet_parameter_rsp param = { 0 };
        void *params[]  = {&param.ref, &param.msg_id, &param.result};

        /* parse param */
        ret = at_parse_param_safe("%d,%d,%d", (char *)data, params);

        if (ret < 0 || ret != (sizeof(params) / sizeof(void *))) {
            LOGE(TAG, "parse");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "ref");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (0 != check_coap_result_code(param.result, RSP_SETPARAMS)) {
            LOGE(TAG, "result");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (cis_response(onenet_context, NULL, NULL, param.msg_id,
                         get_coap_result_code(param.result)) != CIS_RET_OK) {
            LOGE(TAG, "response");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_mipldiscoverresp(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        struct onenet_discover_rsp param = { 0 };
        void *params[5]  = {&param.ref, &param.msg_id, &param.result, &param.length, param.value};

        /* parse param */
        ret = at_parse_param_safe("%d,%d,%d,%d,%" STR(MAX_SET_PARAM_SIZE) "s", (char *)data, params);

        if (ret < 0 || ret != (sizeof(params) / sizeof(void *))) {
            LOGE(TAG, "parse");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "ref");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (0 != check_coap_result_code(param.result, RSP_DISCOVER)) {
            LOGE(TAG, "result");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        if (param.length != strlen(param.value)) {
            LOGE(TAG, "lenght");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (param.result == 1) {
            char *buft = param.value;
            char *lastp = NULL;
            char *strres = NULL;

            while ((strres = strtok_r(buft, ";", &lastp)) != NULL) {
                cis_uri_t uri = {0};
                uri.objectId = URI_INVALID;
                uri.instanceId = URI_INVALID;
                uri.resourceId = atoi(strres);
                cis_uri_update(&uri);
                cis_response(onenet_context, &uri, NULL, param.msg_id, CIS_RESPONSE_CONTINUE);
                buft = NULL;
            }
        }

        if (cis_response(onenet_context, NULL, NULL, param.msg_id,
                         get_coap_result_code(param.result)) != CIS_RET_OK) {
            LOGE(TAG, "response");
            AT_BACK_CIS_ERR(CIS_STATUS_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_miplupdate(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        struct onenet_update param = { 0 };
        void *params[]  = {&param.ref, &param.lifetime, &param.with_obj_flag};

        /* parse param */
        ret = at_parse_param_safe("%d,%d,%d,%" STR(MAX_INS_MAP_SIZE) "s,%d,%d", (char *)data, params);

        if (ret < 0 || ret != (sizeof(params) / sizeof(void *))) {
            LOGE(TAG, "parse");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (onet_have_context(param.ref) != 0) {
            LOGE(TAG, "ref");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (param.lifetime == 0) {
            param.lifetime = LIFETIME_INVALID;
        } else if (param.lifetime >= LIFETIME_LIMIT_MIN) {
            g_lifetime = param.lifetime;
        } else {
            LOGE(TAG, "lifetime");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (!(param.with_obj_flag == 0 || param.with_obj_flag == 1)) {
            LOGE(TAG, "obj_flag");
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        if (cis_update_reg(onenet_context, param.lifetime, param.with_obj_flag) != COAP_NO_ERROR) {
            LOGE(TAG, "reg");
            AT_BACK_CIS_ERR(CIS_UNKNOWN_ERROR);
            return;
        }

        AT_BACK_OK();
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_onet_miplver(char *cmd, int type, char *data)
{
    if (type == READ_CMD) {
        cis_version_t ver;
        char at_str[32] = {0};

        if (cis_version(&ver) != CIS_RET_OK) {
            AT_BACK_CIS_ERR(CIS_PARAM_ERROR);
            return;
        }

        snprintf(at_str, sizeof(at_str), "\r\n+MIPLVER: %x.%x%s",
                 ver.major, ver.minor, AT_RSP_OK);

        AT_BACK_STR(at_str);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

/* register in cop */
/* AT+COPREG=cid.c-sky.com/open.c-sky.com */
void at_cmd_onet_copreg(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;

        //data[len] = 0;
        extern int cop_register(const char *url);
        ret = cop_register((char *)data);

        if (ret == 0) {
            AT_BACK_OK();
            return;
        }

        AT_BACK_ERRNO(ret);
        return;
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }

}

#ifdef CONFIG_YOC_LPM
int32_t at_onenet_lpm_hdl()
{
    // fix onenet ref = 1
    if (onet_have_context(1) != 0) {
        LOGD(TAG, "onet_have_context");
        return 0;
    }

    if (g_shutdown == false) {
        if (g_at_oneopen_hdl.hdl != NULL) {

            while (cis_isregister_enabled(onenet_context)) {
                cis_unregister(onenet_context);
                aos_msleep(100);
            }

            g_shutdown = true;
            aos_sem_wait(&client_opensem, -1);
            aos_sem_free(&client_opensem);
            g_at_oneopen_hdl.hdl = NULL;
            LOGD(TAG, "onet sleep close");
        }
    }

    if (onenet_context != NULL) {
        if (0 == onet_deinit()) {
            LOGD(TAG, "onenet agree sleep");
            aos_lpm_agree_sleep(LPM_PROC_CLOUD_ONENET);
        } else {
            LOGE(TAG, "onenet del failed");
            return -1;
        }
    }

    return 0;
}
#endif
#endif