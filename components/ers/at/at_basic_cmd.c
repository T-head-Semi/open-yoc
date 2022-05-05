/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/kernel.h>
#include <time.h>
#include <aos/version.h>
#include <yoc/nvram.h>
#include <yoc/atserver.h>

#include "at_internal.h"
#include <drv/tee.h>

#include "at_event.h"

#define TAG "at_basic_cmd"


#ifdef CONFIG_YOC_LPM
    #define SLEEP_IN_SOON_EVT_VAL       1
    #define SLEEP_IN_DELAY_EVT_VAL      0
#endif

static uint8_t g_at_event_flg = 1;

void at_cmd_at(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_BACK_OK();
    }
}

void at_cmd_help(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        atserver_send("\r\n%s:", cmd + 2);
        atserver_show_command();
        atserver_send("\r\nOK\r\n");
    }
}

void at_cmd_cgmr(char *cmd, int type, char *data)
{
    if ((type == EXECUTE_CMD) || (type == TEST_CMD)) {
        AT_BACK_RET_OK(cmd, aos_get_os_version());
    }
}

void at_cmd_fwver(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_BACK_RET_OK(cmd, aos_get_os_version());
    }
}

void at_cmd_systime(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        struct timespec ts;

        clock_gettime(CLOCK_MONOTONIC, &ts);
        atserver_send(CRLF "%s:%d.%06d" CRLF_OK_CRLF, cmd + 2, ts.tv_sec, (int)ts.tv_nsec / 1000);
    }
}

void at_cmd_save(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_BACK_RET_ERR(cmd, AT_ERR);
    }
}

void at_cmd_factory_w(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int key_cnt, val_cnt;

        atserver_scanf("%*[^,]%n%*[^\0]%n", &key_cnt, &val_cnt);
        val_cnt -= (key_cnt + 1);

        if (val_cnt < 131 && key_cnt < 20) {
            char key[20 + 2] = {0};
            char val[128 + 3] = {0};

            if (atserver_scanf("\"%[^\"]\",\"%[a-zA-Z0-9_@.:]\"", key, val) == 2) {
                if (val_cnt != strlen(val)) {

                    if (0 == strcmp(key, "PRODUCTKEY")
                        || 0 == strcmp(key, "DEVICENAME")
                        || 0 == strcmp(key, "DEVICESECRET")) {

                        int ret = nvram_set_val(key, val);

                        if (ret > 0) {
                            AT_BACK_OK();
                            return;
                        }
                    }
                }
            }
        }

        AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
    } else if (type == EXECUTE_CMD) {
        AT_BACK_RET_ERR(cmd, AT_ERR);
    }
}

void at_cmd_factory_r(char *cmd, int type, char *data)
{
    if (type == READ_CMD) {
        char item_buf[128 + 1];
        int ret;

        ret = nvram_get_val("PRODUCTKEY", item_buf, sizeof(item_buf));
        atserver_send(CRLF"%s:", cmd + 2);

        if (ret > 0) {
            atserver_send("%s,", item_buf);
        }

        ret = nvram_get_val("DEVICENAME", item_buf, sizeof(item_buf));

        if (ret > 0) {
            atserver_send("%s", item_buf);
        }

        atserver_send(CRLF_OK_CRLF);
    }
}

void at_cmd_reboot(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_BACK_OK();
        aos_reboot();
    }
}

void at_cmd_event_and_echo(char *cmd, int type, char *data, uint8_t *flag)
{
    if (type == READ_CMD) {
        AT_BACK_RET_OK(cmd, *flag ? "ON" : "OFF");
    } else if (type == WRITE_CMD) {
        if (strncmp((const char *)data, "ON", 2) == 0 || strncmp((const char *)data, "OFF", 3) == 0) {
            *flag = (data[1] == 'N') ? 1 : 0;
            AT_BACK_OK();
        } else {
            AT_BACK_RET_ERR(cmd, AT_ERR);
        }
    }
}
void at_cmd_event(char *cmd, int type, char *data)
{
    at_cmd_event_and_echo(cmd, type, data, &g_at_event_flg);
}

void at_cmd_echo(char *cmd, int type, char *data)
{
    uint8_t echo_flag = atserver_get_echo();
    at_cmd_event_and_echo(cmd, type, data, &echo_flag);
    atserver_set_echo(echo_flag);
}

int32_t at_cmd_event_on(void)
{
    return g_at_event_flg;
}

//#if !defined(CONFIG_PRODUCT_MODEL)
void at_cmd_model(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        if (data != NULL) {
            int len = (strlen(data));

            if (len > 0 && len < 64) {
                int ret = nvram_set_val("model", (char *)data);

                if (ret > 0) {
                    AT_BACK_OK();
                    return;
                }
            }
        }

        AT_BACK_ERR();

    } else if (type == READ_CMD) {
        char astr[64];
        int  ret;
        ret = nvram_get_val("model", astr, sizeof(astr));

        if (ret > 0) {
            AT_BACK_RET_OK(cmd, astr);
        } else {
            AT_BACK_ERR();
        }
    }
}
//#endif
#ifdef CONFIG_YOC_LPM

void at_cmd_sleep(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_LOGD(TAG, "at all lpm sleep");
        at_event_send(AT_MSG_EVENT, AT_EVENT_LPM, 0, SLEEP_IN_SOON_EVT_VAL, NULL);
        AT_BACK_OK();
    }
}
#endif
