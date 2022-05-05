/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <yoc/aui_cloud.h>
#include <media.h>
#include <aos/ringbuffer.h>
#include <aos/debug.h>
#include <devices/wifi.h>
#include <aos/kv.h>

#include "cJSON.h"
#include "nui_things.h"
#include "cloud_mit.h"
#include "nls_nui_things_tts.h"

#define TAG "MIT_ASR"

#define MIT_ASR_BUFFER_SIZE 200 * 1024 //50K

#define MIT_ASR_KEY "y5QsLk2A3acWEhCs"
#define MIT_ASR_TOKEN "4a37aa0c0373498ea04f732054841b62"
#define MIT_ASR_URL "wss://nls-gateway-inner.aliyuncs.com/ws/v1"

#define MIT_ASR_TASK_QUIT_EVT (0x01)

mit_account_info_t g_mit_account_info;

static NuiThingsConfig mit_dialog_config = {0};
static char *          asr_buf           = NULL;
// long int test_asr_len = 0;

typedef enum mit_status {
    MIT_STATE_INIT = 0,
    MIT_STATE_ONGOING,
    MIT_STATE_FINISH,
    MIT_STATE_RESULT,
    MIT_STATE_CLOUD_RESULT,
    MIT_STATE_END,
} mit_status_t;

typedef enum mit_asr_err_type {
    MIT_ASR_SUCCESS = 0,
    MIT_ASR_NETWORK_ERR,
    MIT_ASR_PARSE_FAIL,
} mit_asr_err_type;

typedef struct mit_context {
    dev_ringbuf_t    rbuf;
    mit_status_t     status;
    mit_asr_err_type err;
    char *           asr_output;
} mit_context_t;

typedef struct mit_kws_data {
    volatile int do_wwv; // need to do wwv
    volatile int data_valid;
    char *       data;
    size_t       len;
    size_t       pos;
} mit_kws_data_t;

static int            bufferLocked = 0;
static mit_kws_data_t g_mit_kws;
static aos_event_t    asr_task_quit;

static void mit_asr_handle(void *data, int len, void *priv)
{
    aui_kws_t kws_info;
    aui_t *aui = (aui_t *)priv;
    char *json_text = (char *)data;

    if (!g_mit_kws.do_wwv) {
        LOGD(TAG, "%s normal asr\n", __func__);
        kws_info.asr_type = 0;
        kws_info.data     = data;
        kws_info.len      = len;

        if (aui->cb.asr_cb) {
            aui->cb.asr_cb((void *)&kws_info, sizeof(aui_kws_t), aui->cb.asr_priv);
        }
        return;
    }

    cJSON *js = cJSON_Parse(json_text);
    cJSON *state = cJSON_GetObjectItem(js, "aui_kws_result");

    int32_t event = atoi(state->valuestring);
    switch (event) {
        case AUI_KWS_REJECT:
            LOGD(TAG, "wwv rejected");
            kws_info.asr_type = 1;
            kws_info.data     = (char *)&event;
            kws_info.len      = sizeof(int32_t);
            break;

        case AUI_KWS_CONFIRM:
            LOGD(TAG, "wwv confirmed");
            kws_info.asr_type = 1;
            kws_info.data     = (char *)&event;
            kws_info.len      = sizeof(int32_t);
            break;

        default:
            LOGD(TAG, "%s asr\n", __func__);
            kws_info.asr_type = 0;
            kws_info.data     = data;
            kws_info.len      = len;
            break;
    }

    if (aui->cb.asr_cb) {
        aui->cb.asr_cb((void *)&kws_info, sizeof(aui_kws_t), aui->cb.asr_priv);
    }
}

static void get_session_id(char *buff, const int number)
{
    char rand_string[] = "0123456789abcdeffdecba9876543210";
    char ss[3]         = {0};

    /* use random number as last 12 bytes */
    struct timeval time_now; //= {0};
    gettimeofday(&time_now, NULL);
    long time_mil = 0;
    time_mil      = time_now.tv_sec * 1000 + time_now.tv_usec / 1000;
    srand(((unsigned int)time_mil));

    for (int i = 1; i <= number; i++) {
        memset(ss, 0x0, 3);
        sprintf(ss, "%c", rand_string[(int)(32.0 * rand() / (RAND_MAX + 1.0))]);
        strcat(buff, ss);
    }
}

static void get_hex_mac(char *hex_mac)
{
    static uint8_t s_mac[6] = {0};
    int ret, try = 0;

    if (s_mac[0] == 0 && s_mac[1] == 0 && s_mac[2] == 0 && 
        s_mac[3] == 0 && s_mac[4] == 0 && s_mac[5] == 0) {
        aos_dev_t *wifi_dev = NULL;
        wifi_dev = device_open_id("wifi", 0);
        if(NULL == wifi_dev) {
            LOGE(TAG, "open wifi device error!");
            return;
        }

        do {
            ret = hal_wifi_get_mac_addr(wifi_dev, s_mac);
            if (ret == 0) {
                break;
            }

            aos_msleep(500);
        } while (++try < 5);
    }

    if (try == 5) {
        LOGW(TAG, "get mac failed, use default mac");

        s_mac[0] = 0x00;
        s_mac[1] = 0xE0;
        s_mac[2] = 0x90;
        s_mac[3] = 0x60;
        s_mac[4] = 0x30;
        s_mac[5] = 0x12;
    }

    for (int i = 0; i < 6; i++) {
        sprintf(hex_mac + i * 2, "%02x", s_mac[i]);
    }
}

static void get_uuid(char *uuid)
{
    char product_key[32 + 1] = {0};
    char device_name[32 + 1] = {0};
    int pk_len = sizeof(product_key), dn_len = sizeof(device_name);

    int ret1 = aos_kv_get("hal_devinfo_pk", product_key, &pk_len);
    int ret2 = aos_kv_get("hal_devinfo_dn", device_name, &dn_len);
    if (ret1 == 0 && ret2 == 0) {
        sprintf(uuid, "%s&%s", product_key, device_name);
    } else {
        get_hex_mac(uuid);
    }

    LOGD(TAG, "device uuid %s", uuid);
}

static char *mit_asr_get_account(void)
{
    cJSON *js_account_info = NULL;
    char *account_info_p   = NULL;
    char device_uuid[100]  = {0};

    get_uuid(device_uuid);

    js_account_info = cJSON_CreateObject();
    CHECK_RET_WITH_GOTO(js_account_info, END);

    int mitasr_key = 0;
    aos_kv_getint("mitasr", &mitasr_key);

    switch(mitasr_key) {
    case 1:
        /*ASR IoT feiyan*/
        cJSON_AddStringToObject(js_account_info, "device_uuid", device_uuid);
        cJSON_AddStringToObject(js_account_info, "asr_app_key", "DxcfGXG8NLCuH37h");
        cJSON_AddStringToObject(js_account_info, "asr_token", "4a37aa0c0373498ea04f732054841b62");
        cJSON_AddStringToObject(js_account_info, "asr_url", "wss://nls-gateway-inner.aliyuncs.com/ws/v1");
        cJSON_AddStringToObject(js_account_info, "dialog_context", "{\"systemInfo\":\"{\\\"app_package\\\":\\\"com.cibn.tv\\\",\\\"package_name\\\":\\\"com.cibn.tv\\\",\\\"ykPid\\\":\\\"68935a5f396b549b\\\",\\\"uuid\\\":\\\"C55301A5037835E20B07AF9B10697AD9\\\",\\\"device_model\\\":\\\"Konka Android TV 551\\\",\\\"device_system_version\\\":\\\"5.1.1\\\",\\\"device_sn\\\":\\\"C55301A5037835E2\\\",\\\"device_firmware_version\\\":\\\"5.1.1\\\",\\\"firmware\\\":\\\"5.1.1\\\",\\\"charge_type\\\":\\\"2,3,5,7\\\",\\\"sw\\\":\\\"sw1080\\\",\\\"version_code\\\":2120601225,\\\"yingshi_version\\\":2120601225,\\\"com.cibn.tv\\\":2120601225,\\\"device_media\\\":\\\"\\\",\\\"mac\\\":\\\"90C35FB9D08C\\\",\\\"ethmac\\\":\\\"88795B2C38D6\\\",\\\"from\\\":\\\"0,7,9\\\",\\\"license\\\":\\\"7\\\",\\\"bcp\\\":\\\"7\\\",\\\"v_model\\\":\\\"F\\\",\\\"version_name\\\":\\\"6.1.2.25\\\"}\",\"platformKey\":\"\",\"sceneInfo\":\"{\\\"appPackage\\\":\\\"com.konka.athena\\\",\\\"awakenWord\\\":\\\"小康小康\\\",\\\"childVoiceOpen\\\":0,\\\"city\\\":\\\"深圳\\\",\\\"clientVersion\\\":1083,\\\"clientVersionName\\\":\\\"1.0.1083\\\",\\\"deviceMode\\\":0,\\\"media_source\\\":\\\"konka_tencent\\\",\\\"micType\\\":0,\\\"speakerInfoBO\\\":{},\\\"supportChild\\\":0,\\\"useApp\\\":\\\"com.konka.livelauncher\\\",\\\"useAppClientVersion\\\":90008,\\\"vipType\\\":0}\",\"packageInfo\":\"{\\\"com.konka.multimedia\\\":\\\"89649\\\",\\\"com.hisilicon.android.hiRMService\\\":\\\"1\\\",\\\"com.android.defcontainer\\\":\\\"22\\\",\\\"com.konka.message\\\":\\\"100079\\\",\\\"com.konka.konkabtctlbind_5\\\":\\\"82425\\\",\\\"com.konka.applist\\\":\\\"88654\\\",\\\"com.konka.smartengine\\\":\\\"90426\\\",\\\"com.iflytek.showcomesettings\\\":\\\"1159\\\",\\\"com.konka.kksmarthome\\\":\\\"175\\\",\\\"com.gitvkonka.video\\\":\\\"73513\\\",\\\"com.konka.bootlogicproxy\\\":\\\"2\\\",\\\"com.android.inputdevices\\\":\\\"22\\\",\\\"com.tencent.karaoketv\\\":\\\"32\\\",\\\"com.konka.systemsetting\\\":\\\"90086\\\",\\\"com.konka.setupwizard\\\":\\\"89490\\\",\\\"com.konka.mor.tv\\\":\\\"180818\\\",\\\"com.android.externalstorage\\\":\\\"22\\\",\\\"com.konka.vadr\\\":\\\"39815\\\",\\\"com.konka.SmartControl\\\":\\\"20190412\\\",\\\"com.konka.localserver\\\":\\\"89893\\\",\\\"com.konka.hotelmenu\\\":\\\"90227\\\",\\\"com.android.keychain\\\":\\\"22\\\",\\\"com.konka.downloadcenter\\\":\\\"102\\\",\\\"com.konka.cloudsearch\\\":\\\"88623\\\",\\\"com.konka.fourkshow\\\":\\\"90060\\\",\\\"com.android.managedprovisioning\\\":\\\"22\\\",\\\"com.iflytek.xiri.ime\\\":\\\"10493\\\",\\\"com.konka.kkfactory\\\":\\\"89882\\\",\\\"com.konka.systemadvert\\\":\\\"88999\\\",\\\"com.iflytek.itvs\\\":\\\"20110\\\",\\\"com.konka.livelauncher\\\":\\\"90008\\\",\\\"com.bestv.mishitong.tv\\\":\\\"23081306\\\",\\\"com.shafa.konka.appstore\\\":\\\"401\\\",\\\"com.ktcp.tvvideo\\\":\\\"3720\\\",\\\"com.konka.kksystemui\\\":\\\"90084\\\",\\\"com.xiaodianshi.tv.yst\\\":\\\"101602\\\",\\\"com.konka.tvsettings\\\":\\\"90155\\\",\\\"com.konka.passport\\\":\\\"80571\\\",\\\"android\\\":\\\"22\\\",\\\"com.yunos.tvtaobao\\\":\\\"2110500004\\\",\\\"com.konka.familycontrolcenter\\\":\\\"72300\\\",\\\"com.konka.quickstandby\\\":\\\"1\\\",\\\"com.android.webview\\\":\\\"399992\\\",\\\"com.dianshijia.newlive\\\":\\\"335\\\",\\\"com.android.providers.settings\\\":\\\"22\\\",\\\"com.android.systemui\\\":\\\"22\\\",\\\"com.yunos.tv.appstore\\\":\\\"2101403003\\\",\\\"hdpfans.com\\\":\\\"93\\\",\\\"com.konka.appupgrade\\\":\\\"1000199\\\",\\\"com.konka.activitycontainer\\\":\\\"788\\\",\\\"com.konka.a2dpsink\\\":\\\"10904\\\",\\\"com.konka.athena\\\":\\\"1083\\\",\\\"com.konka.tvmall\\\":\\\"38586\\\",\\\"com.konka.account\\\":\\\"83534\\\",\\\"com.konka.market.main\\\":\\\"90000\\\",\\\"com.bajintech.karaok\\\":\\\"1031\\\",\\\"com.kangjia.dangbeimarket\\\":\\\"132\\\",\\\"com.android.packageinstaller\\\":\\\"81283\\\",\\\"com.iflytek.showcome\\\":\\\"20433\\\",\\\"com.android.bluetooth\\\":\\\"22\\\",\\\"com.konka.videorecords\\\":\\\"89981\\\",\\\"com.android.shell\\\":\\\"22\\\",\\\"com.konka.kkmultiscreen\\\":\\\"90152\\\",\\\"com.konka.upgrade\\\":\\\"100206\\\",\\\"com.iflytek.xiri\\\":\\\"190000001\\\",\\\"com.konka.adverttool\\\":\\\"89495\\\",\\\"com.konka.tvmanager\\\":\\\"89701\\\",\\\"com.cibn.tv\\\":\\\"2120601225\\\",\\\"com.tencent.qqmusictv\\\":\\\"311\\\",\\\"com.konka.systeminfo\\\":\\\"190226\\\"}\"}");
        break;
    case 2:
        /*ASR kaishu*/
        cJSON_AddStringToObject(js_account_info, "device_uuid", device_uuid);
        cJSON_AddStringToObject(js_account_info, "asr_app_key", "y5QsLk2A3acWEhCs");
        cJSON_AddStringToObject(js_account_info, "asr_token", "4a37aa0c0373498ea04f732054841b62");
        cJSON_AddStringToObject(js_account_info, "asr_url", "wss://nls-gateway-inner.aliyuncs.com/ws/v1");
        break;
    case 3:
        /*ASR IoT meeting*/
        cJSON_AddStringToObject(js_account_info, "device_uuid", "a1IB0paJvIz&H000029J00000034");
        cJSON_AddStringToObject(js_account_info, "asr_app_key", "d119971b");
        cJSON_AddStringToObject(js_account_info, "asr_token", "df28a632ca2e41d38db53ddf4957e573");
        cJSON_AddStringToObject(js_account_info, "asr_url", "wss://smarth.alibaba-inc.com/ws/v1");
        break;
    default:
        /*ASR kaishu test account*/
        cJSON_AddStringToObject(js_account_info, "device_uuid", device_uuid);
        cJSON_AddStringToObject(js_account_info, "asr_app_key", "g3aHMdL7v63bZCS3");
        cJSON_AddStringToObject(js_account_info, "asr_token", "4a37aa0c0373498ea04f732054841b62");
        cJSON_AddStringToObject(js_account_info, "asr_url", "wss://nls-gateway-inner.aliyuncs.com/ws/v1");
        ;
    }

    /*TTS*/
    cJSON_AddStringToObject(js_account_info, "tts_app_key", "9a7f47f2");
    cJSON_AddStringToObject(js_account_info, "tts_token", "a2f8b80e04f14fdb9b7c36024fb03f78");
    cJSON_AddStringToObject(js_account_info, "tts_url", "wss://nls-gateway-inner.aliyuncs.com/ws/v1");
   
    account_info_p = cJSON_PrintUnformatted(js_account_info);
    CHECK_RET_TAG_WITH_GOTO(account_info_p, END);

END:
    cJSON_Delete(js_account_info);

    return account_info_p;
}

static int mit_asr_event_cb(void *user_data, NuiThingsEvent event, int dialog_finish)
{
    int   ret = 0;
    char  nui_things_info[1024];
    int   size;
    char *data;

    aui_t *        aui     = (aui_t *)user_data;
    mit_context_t *context = (mit_context_t *)aui->ops.asr->priv;

    LOGD(TAG, "call %s(>>event: %d(%s)<<)", __FUNCTION__, event, nui_things_event_get_str(event));

    switch (event) {
        case kNuiThingsEventWwv:
            nui_things_info_print(kNuiThingsInfoTypeWwv);
            memset(nui_things_info, 0, sizeof(nui_things_info));

            ret = nui_things_info_get(kNuiThingsInfoTypeWwv, nui_things_info,
                                      sizeof(nui_things_info));
            if (0 == ret) {
                printf("get wwv info %s\n", nui_things_info);
            } else {
                printf("get wwv info error:%d\n", ret);
            }
            break;
        case kNuiThingsEventVadStart:
            break;
        case kNuiThingsEventVadEnd:
            // context->status = MIT_STATE_END;
            break;

        case kNuiThingsEventAsrPartialResult:
            /* do tts connect in advance to reduce dialog delay */
            aui_cloud_start_tts(aui);
            break;

        case kNuiThingsEventAsrResult:
            context->status =
                context->status == MIT_STATE_FINISH ? MIT_STATE_RESULT : MIT_STATE_CLOUD_RESULT;
            LOGD(TAG, "asr result %s", context->status == MIT_STATE_RESULT ? "local" : "cloud");

            size = nui_things_info_get_length(kNuiThingsInfoTypeAsr);
            if (size < 0) {
                LOGE(TAG, "nui things len1 error %d", size);
                context->err = MIT_ASR_PARSE_FAIL;
                return ret;
            }

            data = (char *)aos_malloc(size + 1);
            aos_check_mem(data);

            data[size] = 0;
            ret        = nui_things_info_get(kNuiThingsInfoTypeAsr, data, size);
            if (0 == ret) {
                LOGD(TAG, "get asr info %s\n", data);
                mit_asr_handle((void *)data, strlen(data), aui->cb.asr_priv);
            }

            free(data);
            break;

        case kNuiThingsEventDialogResult:
            if (context->status == MIT_STATE_RESULT || context->status == MIT_STATE_CLOUD_RESULT) {
                size = nui_things_info_get_length(kNuiThingsInfoTypeDialog);
                if (size < 0) {
                    LOGE(TAG, "nui things len2 error %d", size);
                    context->err = MIT_ASR_PARSE_FAIL;
                    goto END_CB;
                }

                data = (char *)aos_malloc(size + 1);
                aos_check_mem(data);

                data[size] = 0;
                ret        = nui_things_info_get(kNuiThingsInfoTypeDialog, data, size);
                if (0 == ret) {
                    LOGE(TAG, "get dialog info %s\n", data);
                } else {
                    LOGE(TAG, "get dialog info error:%d\n", ret);
                    context->err = MIT_ASR_PARSE_FAIL;
                    goto END_CB;
                }

                mit_asr_handle((void *)data, strlen(data), aui->cb.asr_priv);

                free(data);
            } else {
                context->err = MIT_ASR_PARSE_FAIL;
            }

        END_CB:
            context->status = MIT_STATE_END;
            break;

        case kNuiThingsEventWwvConfirm:
            {
                LOGD(TAG, "kws voice confirmed");
                char text[60] = {0};
                snprintf(text, 60, "{\"aui_kws_result\":%d}", AUI_KWS_CONFIRM);
                mit_asr_handle((void *)text, strlen(text), aui->cb.asr_priv);
            }
            break;

        case kNuiThingsEventWwvReject:
            {
                LOGD(TAG, "mit %s event error", "wwvreject");
                char text[60] = {0};
                snprintf(text, 60, "{\"aui_kws_result\":%d,\"msg\":宝拉宝拉}", AUI_KWS_REJECT);
                mit_asr_handle((void *)text, strlen(text), aui->cb.asr_priv);
            }
            break;

        case kNuiThingsEventAsrError:
            aui_cloud_stop_tts(aui);

            if (context->status == MIT_STATE_CLOUD_RESULT || context->status == MIT_STATE_ONGOING) {
                const char *text = "{\"aui_result\":-1,\"msg\":\"asr parse error\"}";
                mit_asr_handle((void *)text, strlen(text), aui->cb.asr_priv);
            }
            LOGD(TAG, "mit %s event error", "asr");
            context->err    = MIT_ASR_PARSE_FAIL; // TODO assign different error type
            context->status = MIT_STATE_END;
            break;

        default:
            break;
    }

    return ret;
}

static int mit_wwv_provide_data_cb(void *user_data, NuiThingsVoiceData *data)
{
    int ret;

    if (!g_mit_kws.do_wwv) {
        return -1;
    }

    if (!g_mit_kws.data_valid) {
        for (int i = 0; i < 30; i++) {
            aos_msleep(20);
            if (g_mit_kws.data_valid) {
                break;
            }
        }

        if (!g_mit_kws.data_valid) {
            g_mit_kws.do_wwv = 0;
            LOGE(TAG, "get wwv data failed");
            return -1;
        }
    }

    if (data->mode == kNuiThingsVoiceModeKws && data->buffer && data->len > 0) {
        //   LOGD(TAG, "wwv data cb: %d total:%d, offset:%d, need_len:%d", g_mit_kws.valid, g_mit_kws.len, g_mit_kws.pos,  data->len);
        if (g_mit_kws.pos + data->len < g_mit_kws.len) {
            memcpy(data->buffer, g_mit_kws.data + g_mit_kws.pos, data->len);
            g_mit_kws.pos += data->len;
            data->finish = 0;
            ret          = data->len;
        } else {
            memcpy(data->buffer, g_mit_kws.data + g_mit_kws.pos, g_mit_kws.len - g_mit_kws.pos);
            data->finish = 1;
            ret          = g_mit_kws.len - g_mit_kws.pos;

            g_mit_kws.data_valid = 0;
            g_mit_kws.do_wwv     = 0;
            LOGD(TAG, "wwv data end.");
        }
    } else {
        g_mit_kws.data_valid = 0;
        g_mit_kws.do_wwv     = 0;
        ret                  = -1;
        LOGW(TAG, "wwv provide data cb err");
    }

    return ret;
}

static int mit_asr_provide_data_cb(void *user_data, char *buffer, int len)
{
    int            ret     = 0;
    aui_t *        aui     = (aui_t *)user_data;
    mit_context_t *context = (mit_context_t *)aui->ops.asr->priv;
    // LOGD(TAG, "call %s(%p, %d) [start]",__FUNCTION__, buffer, len);

    if (context->status == MIT_STATE_ONGOING) {
        for (int i = 0; i < 2 && ringbuffer_available_read_space(&context->rbuf) < len; i++) {
            aos_msleep(40);
        }

        if (ringbuffer_available_read_space(&context->rbuf) < len) {
            return 0;
        }
    }

    ret = ringbuffer_read(&context->rbuf, (uint8_t *)buffer, len);
    // LOGD(TAG, "provide data cb read %d bytes", len);

    return ret;
}

static void tsk_wait_for_mit_consuming(void *arg)
{
    aui_t *        aui         = (aui_t *)arg;
    mit_context_t *context     = (mit_context_t *)aui->ops.asr->priv;
    int            timeout_cnt = 0;

    // wait for data streaming finish, timeout in 5 s
    LOGD(TAG, "Wait consumig thread");
    while (context->status != MIT_STATE_END && context->err == MIT_ASR_SUCCESS &&
           ringbuffer_available_read_space(&context->rbuf) > 0) {
        aos_msleep(50);

        if (++timeout_cnt > 200)
            break;
    }

    if (context->status != MIT_STATE_END && context->err == MIT_ASR_SUCCESS) {
        // stop the mit asr to get the final result
        LOGD(TAG, "stop mit asr");
        nui_things_stop(0);

        timeout_cnt = 0;
        while (context->status != MIT_STATE_END) {
            aos_msleep(200);
            if (++timeout_cnt == 100) {
                // wait timeout, deem as error
                context->err = MIT_ASR_PARSE_FAIL;
                break;
            }
        }
    }

    context->status = MIT_STATE_END;
    if (context->err != MIT_ASR_SUCCESS) {
        // LOGE(TAG, "MIT asr error %d", context->err);
        nui_things_stop(1);

        LOGD(TAG, "result error %d", context->err);
        switch (context->err) {
            case MIT_ASR_NETWORK_ERR:
                if (aui->cb.asr_cb) {
                    const char *data = "{\"aui_result\":-101,\"msg\":\"mit cloud connect fail(ENETUNREACH)\"}";
                    // aui->cb.asr_cb((void *)data, strlen(data), aui->cb.asr_priv);
                    mit_asr_handle((void *)data, strlen(data), aui->cb.asr_priv);
                }

                break;
            case MIT_ASR_PARSE_FAIL:
                if (aui->cb.asr_cb) {
                    const char *data = "{\"aui_result\":-1,\"msg\":\"asr parse error\"}";
                    // aui->cb.asr_cb((void *)data, strlen(data), aui->cb.asr_priv);
                    mit_asr_handle((void *)data, strlen(data), aui->cb.asr_priv);
                }
                break;
            default:
                break;
        }
    }

    LOGD(TAG, "consumig thread end");
    aos_event_set(&asr_task_quit, MIT_ASR_TASK_QUIT_EVT, AOS_EVENT_OR);
    return;
}

static int nui_things_return_data_main(void *              user_data,
                                       NuiThingsVoiceData *data /*char * buffer, int len*/)
{
    int ret = 0;
    LOGD(TAG, "call %s(len=%d finish=%d) [start]", __FUNCTION__, data->len, data->finish);

    switch (data->mode) {
        case kNuiThingsVoiceModeKws:
            break;
        case kNuiThingsVoiceModeAsr:
            break;
        default:
            break;
    }
    LOGD(TAG, "call %s() [done]", __FUNCTION__);
    return ret;
}

static int mit_asr_set_account(aui_t *aui)
{
    cJSON *j_info            = NULL;

    aos_check_param(aui);

    aui->config.js_account = mit_asr_get_account();
    LOGD(TAG, "mit_asr_set_account json_account_info: %s", aui->config.js_account);

    j_info                = cJSON_Parse(aui->config.js_account);

    cJSON *device_uuid    = cJSON_GetObjectItem(j_info, "device_uuid");
    cJSON *asr_app_key    = cJSON_GetObjectItem(j_info, "asr_app_key");
    cJSON *asr_token      = cJSON_GetObjectItem(j_info, "asr_token");
    cJSON *asr_url        = cJSON_GetObjectItem(j_info, "asr_url");
    cJSON *tts_app_key    = cJSON_GetObjectItem(j_info, "tts_app_key");
    cJSON *tts_token      = cJSON_GetObjectItem(j_info, "tts_token");
    cJSON *tts_url        = cJSON_GetObjectItem(j_info, "tts_url");
    cJSON *tts_key_id     = cJSON_GetObjectItem(j_info, "tts_key_id");
    cJSON *tts_key_secret = cJSON_GetObjectItem(j_info, "tts_key_secret");
    cJSON *dialog_context = cJSON_GetObjectItem(j_info, "dialog_context");

    CHECK_RET_TAG_WITH_GOTO(j_info && device_uuid && cJSON_IsString(device_uuid) && asr_app_key &&
                                cJSON_IsString(asr_app_key) && asr_token &&
                                cJSON_IsString(asr_token) && asr_url && cJSON_IsString(asr_url) &&
                                tts_app_key && cJSON_IsString(tts_app_key) && tts_token &&
                                cJSON_IsString(tts_token) && tts_url && cJSON_IsString(tts_url),
                            ERR);

    g_mit_account_info.device_uuid    = device_uuid->valuestring;
    g_mit_account_info.asr_app_key    = asr_app_key->valuestring;
    g_mit_account_info.asr_token      = asr_token->valuestring;
    g_mit_account_info.asr_url        = asr_url->valuestring;
    g_mit_account_info.tts_app_key    = tts_app_key->valuestring;
    g_mit_account_info.tts_token      = tts_token->valuestring;
    g_mit_account_info.tts_url        = tts_url->valuestring;
    if (dialog_context) {
        g_mit_account_info.dialog_context = dialog_context->valuestring;
    }

    if ((tts_key_id && cJSON_IsString(tts_key_id)) &&
        (tts_key_secret && cJSON_IsString(tts_key_secret))) {
        g_mit_account_info.tts_key_id     = tts_key_id->valuestring;
        g_mit_account_info.tts_key_secret = tts_key_secret->valuestring;
    }

    mit_dialog_config.device_uuid        = g_mit_account_info.device_uuid;
    mit_dialog_config.app_key            = g_mit_account_info.asr_app_key;
    mit_dialog_config.token              = g_mit_account_info.asr_token;
    mit_dialog_config.url                = g_mit_account_info.asr_url;
    mit_dialog_config.dialog_context     = g_mit_account_info.dialog_context;
    mit_dialog_config.enable_vad_cloud   = aui->config.cloud_vad; //1;//enable cloud nn vad
    mit_dialog_config.enable_decoder_vad = 1;                     //1;// enable cloud decoder vad
    return 0;

ERR:
    if (j_info) {
        cJSON_Delete(j_info);
    }

    return -1;
}

/** init only once with multiple talks */
static int mit_asr_init(aui_t *aui)
{
    aos_check_return_einval(aui);

    // aui->config.per            = "aixia";
    // aui->config.srate          = 16000;      /* 采样率，16000 */
    // aui->config.vol            = 100;        /* 音量 0~100 */
    // aui->config.spd            = 0;          /* -500 ~ 500*/
    // aui->config.pit            = 0;          /* 音调*/
    // aui->config.asr_fmt        = 2;          /* 编码格式，1：PCM 2：MP3 */
    // aui->config.tts_cache_path = NULL;       /* TTS内部缓存路径，NULL：关闭缓存功能 */
    // aui->config.tts_fmt        = 2;          /* 编码格式，1：PCM 2：MP3 */
    aui->config.cloud_vad      = 1;          /* 云端VAD功能使能， 0：关闭；1：打开 */

    mit_context_t *context = aos_malloc(sizeof(mit_context_t));
    aos_check_mem(context);

    asr_buf = aos_zalloc(MIT_ASR_BUFFER_SIZE);
    aos_check_mem(asr_buf);

    ringbuffer_create(&context->rbuf, asr_buf, MIT_ASR_BUFFER_SIZE);
    context->err         = MIT_ASR_SUCCESS;
    aui->ops.asr->priv   = context;
    context->status      = MIT_STATE_END;

    g_mit_kws.do_wwv     = 0;
    g_mit_kws.data_valid = 0;
    g_mit_kws.pos        = 0;

    /* mit sdk struct */
    NuiThingsListener   mit_listener;
    NuiThingsInitConfig mit_init_config; //init 配置

    memset(&mit_init_config, 0, sizeof(mit_init_config));
    mit_listener.on_event_callback      = mit_asr_event_cb;
    mit_listener.need_data_callback     = mit_asr_provide_data_cb;
    mit_listener.put_data_callback      = nui_things_return_data_main;
    mit_listener.need_data_callback_wwv = mit_wwv_provide_data_cb;
    mit_listener.need_data_callback_nls = NULL;
    mit_listener.user_data              = aui;
    mit_init_config.mode                = kNuiThingsModeNls;
    mit_init_config.listener            = &mit_listener;
    mit_init_config.enable_fe           = 0;
    mit_init_config.enable_kws          = 0;
    mit_init_config.enable_vad          = 1;
    mit_init_config.log_level           = 4;
    mit_init_config.log_link_enable     = 0;

    mit_asr_set_account(aui);

    if (!mit_dialog_config.session_id) {
        mit_dialog_config.session_id = (char *)aos_zalloc_check(32 + 1);
    }

    get_session_id(mit_dialog_config.session_id, 32);
    // log_setLevel(1);//Warning

extern int g_silence_log_level;
    g_silence_log_level=5;

    int ret;
    ret = aos_event_new(&asr_task_quit, 0);
    CHECK_RET_TAG_WITH_RET(ret == 0, -1);

    return nui_things_init(&mit_init_config);
}

static int mit_start_pcm(aui_t *aui)
{
    aos_check_return_einval(aui);

    if (aui->asr_type == 1) {
        LOGI(TAG, "enable wwv in cloud!");
        mit_dialog_config.enable_wwv = 1;
        mit_dialog_config.kws_format = "pcm";
        mit_dialog_config.kws_model  = "gushiji-baola";
        mit_dialog_config.kws_word   = "宝拉宝拉";
        g_mit_kws.do_wwv             = 1;
    } else {
        mit_context_t *context = (mit_context_t *)aui->ops.asr->priv;

        mit_dialog_config.enable_wwv = 0;
        g_mit_kws.do_wwv             = 0;
        g_mit_kws.data_valid         = 0;
        aos_event_set(&asr_task_quit, 0, AOS_EVENT_AND);

        mit_status_t stat = context->status;
        if (stat != MIT_STATE_END) {
            unsigned int flag;

            context->status = MIT_STATE_END;
            nui_things_stop(1);

            if (stat == MIT_STATE_FINISH) {
                aos_event_get(&asr_task_quit, MIT_ASR_TASK_QUIT_EVT, AOS_EVENT_OR_CLEAR, &flag,
                            AOS_WAIT_FOREVER);
            }
        }

        // clear the ring buffer space
        ringbuffer_clear(&context->rbuf);

        LOGD(TAG, "session_id=%s", mit_dialog_config.session_id);

        if (0 != nui_things_start(&mit_dialog_config)) {
            LOGW(TAG, "nui_things_start return error");
            context->err    = MIT_ASR_NETWORK_ERR;
            context->status = MIT_STATE_END;
            nui_things_stop(1);
            return -1;
        }

        aui_cloud_stop_tts(aui);

        context->status = MIT_STATE_ONGOING;
        context->err    = MIT_ASR_SUCCESS;
        bufferLocked    = 0;

        LOGD(TAG, "nui_things_start success");        
    }

    return 0;
}

static int mit_push_pcm(aui_t *aui, void *data, size_t size)
{
    aos_check_return_einval(aui && data && size);

    int ret = 0;

    if (aui->asr_type == 1) {
        g_mit_kws.data_valid = 1;
        g_mit_kws.data       = data;
        g_mit_kws.len        = size;
        g_mit_kws.pos        = 0;
        aui->asr_type        = 0;
    } else {
        mit_context_t *context = (mit_context_t *)aui->ops.asr->priv;
        ret                    = -1;
        if (context->status != MIT_STATE_ONGOING) {
            return 0;
        }

        if (context->err != MIT_ASR_SUCCESS) {
            ret = -1;
            goto END_SOURCE_PCM;
        }

        if (bufferLocked || ringbuffer_full(&context->rbuf)) {
            if (!bufferLocked) {
                LOGD(TAG, "buffer locked");
            }
            bufferLocked = 1;
            return 0;
        }

        ringbuffer_write(&context->rbuf, (uint8_t *)data, size);
        // LOGD(TAG, "mit asr buf left %d", mit_rbuf_available_space(&context->rbuf));

        return 0;

    END_SOURCE_PCM:
        if (ret < 0)
            LOGE(TAG, "Source PCM Error\n");        
    }

    return ret;
}

static int mit_stop_pcm(aui_t *aui)
{
    aos_check_return_einval(aui);
    if (aui->asr_type == 1) {
    
    } else {
        mit_context_t *context = (mit_context_t *)aui->ops.asr->priv;

        if (context->status == MIT_STATE_RESULT) {
            return 0;
        } else if (context->status != MIT_STATE_ONGOING) {
            return -1;
        }

        context->status = MIT_STATE_FINISH;

        // create a task to wait for streaming the rest of the buffer
        aos_task_t task_handle;
        if (0 != aos_task_new_ext(&task_handle, "wait_mit", tsk_wait_for_mit_consuming, aui, 8 * 1024,
                                AOS_DEFAULT_APP_PRI)) {
            LOGE(TAG, "Create tsk_wait_for_mit_consuming failed.");
            return -1;
        }

        LOGD(TAG, "MIT source pcm finish");        
    }

    return 0;
}

static int mit_force_stop(aui_t *aui)
{
    aos_check_return_einval(aui);
    if (aui->asr_type == 1) {

    } else {
        mit_context_t *context = (mit_context_t *)aui->ops.asr->priv;
        mit_status_t   stat    = context->status;
        unsigned int   flags;

        g_mit_kws.do_wwv     = 0;
        g_mit_kws.data_valid = 0;
        context->status      = MIT_STATE_END;
        nui_things_stop(1);

        if (stat == MIT_STATE_FINISH) {
            aos_event_get(&asr_task_quit, MIT_ASR_TASK_QUIT_EVT, AOS_EVENT_OR_CLEAR, &flags,
                        AOS_WAIT_FOREVER);
        }
    }

    return 0;
}

#if 1
// FIXME: just for compile
// 因为mit库里面需要用到这个函数
#include <mbedtls/ssl.h>
void mbedtls_ssl_init_ext(mbedtls_ssl_context *ssl, int len)
{
    mbedtls_ssl_init(ssl);
}
#endif

static aui_asr_cls_t mit_asr_cls = {
    .init           = mit_asr_init,
    .start          = mit_start_pcm,
    .push_data      = mit_push_pcm,
    .stop_push_data = mit_stop_pcm,
    .stop           = mit_force_stop
};

void aui_asr_register(aui_t *aui, aui_asr_cb_t cb, void *priv)
{
    aos_check_param(aui);
    aui_cloud_asr_register(aui, &mit_asr_cls, cb, priv);
}
