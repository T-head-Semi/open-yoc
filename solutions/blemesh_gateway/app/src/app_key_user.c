/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <yoc/button.h>
#include "app_main.h"
#include "app_sys.h"
#include "board_config.h"

#define TAG "keyusr"

extern int wifi_prov_method;

typedef enum {
    button_id0 = 0,
} button_id_t;

typedef enum {
    nothing = 1,
    network,
} button_event_t;

void button_evt(int event_id, void *priv)
{
    LOGD(TAG, "button(%s)\n", (char *)priv);
    switch (event_id) {
		case network:
			LOGE(TAG, "ble prov");
			aos_kv_setint("wprov_method", wifi_prov_method);
			if (wifi_prov_method == WIFI_PROVISION_SL_BLE) {
				printf("wifi_prov_method=WIFI_PROVISION_SL_BLE\n");
				aos_kv_del("AUTH_AC_AS");
				aos_kv_del("AUTH_KEY_PAIRS");
			}
			app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);//zz
			aos_reboot();
			break;
		case nothing:
			//TODO
			break;
		default:
			break;
    }
}

void app_button_init(void)
{
    button_init();
    button_add_gpio(button_id0, 0, LOW_LEVEL);//添加gpio按键

    button_evt_t b_tbl[] = {
        {
            .button_id  = button_id0,
            .event_id   = BUTTON_PRESS_UP,//1
            .press_time = 0,
        }
    };
	button_add_event(nothing, b_tbl, sizeof(b_tbl)/sizeof(button_evt_t), button_evt, "nothing");//为当前按键表参数 添加事件
	
	b_tbl[0].button_id = button_id0;
	b_tbl[0].event_id = BUTTON_PRESS_LONG_DOWN;//2
	b_tbl[0].press_time = 2000;
	button_add_event(network, b_tbl, sizeof(b_tbl)/sizeof(button_evt_t), button_evt, "network");//为当前按键表参数 添加事件
}
