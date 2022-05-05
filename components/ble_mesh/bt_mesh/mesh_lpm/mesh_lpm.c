/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_GLP)
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <aos/hal/gpio.h>
#include <mesh_sal_lpm.h>
#include <mesh_lpm.h>
//#include "inc/adv.h"

#if defined(BOARD_PHY6220_EVB)
#include "pm.h"
#endif


#ifndef false
#define false (0)
#endif

#ifndef true
#define true (1)
#endif


#if defined(CONFIG_MESH_LPM) && CONFIG_MESH_LPM > 0

#define TAG "MESH_LPM"

#define IS_RESUME_SCAN(mode)  ((mode) == MESH_LPM_MODE_RX_TX ? 1 : 0)

typedef struct _mesh_lpm_ctx_s {
    mesh_lpm_conf_t p_config;
    uint8_t status :1;
    uint8_t is_mesh_init :1;
    uint8_t is_mesh_lpm_init :1;
    uint8_t is_mesh_lpm_start :1;
    uint8_t is_mesh_lpm_disabled :1;
    aos_timer_t wakeup_timer;
    aos_timer_t io_wakeup_timer;
    long long last_interrupt_time;
} mesh_lpm_ctx_t;




static mesh_lpm_ctx_t mesh_lpm_ctx;

#ifndef MESH_WAKE_UP_IO_NUM_MAX
#define MESH_WAKE_UP_IO_NUM_MAX 5
#endif

mesh_lpm_wakeup_io_config_t g_wakeup_io[MESH_WAKE_UP_IO_NUM_MAX];

static uint8_t g_wakeup_io_num = 0;

#ifndef CONFIG_MESH_ENABLE_SLEEP_WHEN_NOT_PROV
#define CONFIG_MESH_ENABLE_SLEEP_WHEN_NOT_PROV 0
#endif

#define DEF_LPM_MODE   MESH_LPM_MODE_TX_RX

#define DEF_IO_ENTER_LPM_RETRY (1000) //ms


extern int bt_mesh_scan_enable(void);
extern int bt_mesh_scan_disable(void);
extern int bt_mesh_suspend_lpm(bool force);
extern int bt_mesh_resume_lpm(uint8_t scan_enable);


#if defined(CONFIG_PM_SLEEP) && CONFIG_PM_SLEEP > 0

static void _mesh_lpm_timer_irq_handler(void *p_timer, void *args)
{
    int ret = 0;
    static uint8_t first_entry = 0;

    LOGD(TAG,"cur status: %d", mesh_lpm_ctx.status);

    if (mesh_lpm_ctx.is_mesh_lpm_disabled == 1) {
        LOGD(TAG,"lpm already disable, stop timer\n");
        aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
        return;
    }

    if(!first_entry) { //enter lpm mode first
        aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
        mesh_sal_sleep_enable();
        ret = bt_mesh_suspend_lpm(true);
        if (0 == ret || -EALREADY == ret) {
            mesh_lpm_ctx.status = STATUS_SLEEP;
            if(mesh_lpm_ctx.p_config.lpm_wakeup_mode == WAKEUP_BY_TIMER_MODE || mesh_lpm_ctx.p_config.lpm_wakeup_mode == WAKEUP_BY_IO_TIMER_MODE) {
                aos_timer_change(&mesh_lpm_ctx.wakeup_timer, mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.sleep_ms);
                aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
            }//only the IO mode will nerver use the timer again
            first_entry = 1;
        } else {
            LOGD(TAG,"First suspend mesh stack faild %d",ret);
            mesh_lpm_ctx.status = STATUS_WAKEUP;
            if(mesh_lpm_ctx.p_config.lpm_wakeup_mode == WAKEUP_BY_TIMER_MODE || mesh_lpm_ctx.p_config.lpm_wakeup_mode == WAKEUP_BY_IO_TIMER_MODE) {
                aos_timer_change(&mesh_lpm_ctx.wakeup_timer, mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.wakeup_ms);
                aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
                first_entry = 1;
            } else {
                aos_timer_change(&mesh_lpm_ctx.wakeup_timer, DEF_IO_ENTER_LPM_RETRY);
                aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
                first_entry = 0;
            }

        }
        return ;
    }

    if (STATUS_WAKEUP == mesh_lpm_ctx.status) {
        LOGD(TAG,"sleep and suspend mesh stack");
        ret = bt_mesh_suspend_lpm(false);
        if (0 == ret || -EALREADY == ret) {
            mesh_lpm_ctx.status = STATUS_SLEEP;
            if (!mesh_lpm_ctx.p_config.mesh_lpm_cb) {
                mesh_lpm_ctx.p_config.mesh_lpm_cb(WAKEUP_BY_TIMER, mesh_lpm_ctx.status, NULL);
            }
            aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
            aos_timer_change(&mesh_lpm_ctx.wakeup_timer, mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.sleep_ms);
            aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
        } else {
            aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
            aos_timer_change(&mesh_lpm_ctx.wakeup_timer, mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.wakeup_ms);
            aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
        }
    } else if (STATUS_SLEEP == mesh_lpm_ctx.status) {

        LOGD(TAG,"wake up and resume mesh stack");
        ret = bt_mesh_resume_lpm(IS_RESUME_SCAN(mesh_lpm_ctx.p_config.lpm_mode));
        if (0 == ret || -EALREADY == ret) {
            mesh_lpm_ctx.status = STATUS_WAKEUP;
            mesh_lpm_ctx.p_config.mesh_lpm_cb(WAKEUP_BY_TIMER, mesh_lpm_ctx.status, NULL);
            aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
            aos_timer_change(&mesh_lpm_ctx.wakeup_timer, mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.wakeup_ms);
            aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
        } else {
            aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
            aos_timer_change(&mesh_lpm_ctx.wakeup_timer, mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.sleep_ms);
            aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
        }
    }

}
#endif


__attribute__((section(".__sram.code.mesh_lpm_io_wakeup_handler"))) void mesh_lpm_io_wakeup_handler(void *arg)
{
    if (mesh_lpm_ctx.p_config.lpm_wakeup_mode == WAKEUP_BY_IO_MODE || mesh_lpm_ctx.p_config.lpm_wakeup_mode == WAKEUP_BY_IO_TIMER_MODE) {
        uint8_t trigger_found = 0;
        _mesh_lpm_io_status_t g_io_status[MESH_WAKE_UP_IO_NUM_MAX];
        long long interrupt_time = aos_now_ms();

        if ((interrupt_time - mesh_lpm_ctx.last_interrupt_time < INTERRUPT_DELAY_TIME)) {
            return; //for debounce
        }

        mesh_lpm_ctx.last_interrupt_time = interrupt_time;

        for (int i = 0; i < g_wakeup_io_num; i++) {
            bool status = mesh_sal_sleep_wakeup_io_get_status(g_wakeup_io[i].port);
            if (((g_wakeup_io[i].io_pol == FALLING || g_wakeup_io[i].io_pol == ACT_LOW) && (status == false)) ||   \
				((g_wakeup_io[i].io_pol == RISING || g_wakeup_io[i].io_pol == ACT_HIGH) && (status == true))) {
                g_io_status[i].port = g_wakeup_io[i].port;
                g_io_status[i].trigger_flag = 1;
                g_io_status[i].status = status;
                trigger_found = 1;
            }
        }

        if (trigger_found) {
            //LOGD(TAG,"[%u]wakeup by i/o\n", k_uptime_get_32());
            //LOGD(TAG,"[%u]cur status: %d\n", k_uptime_get_32(), mesh_lpm_ctx.status);
            if (STATUS_SLEEP == mesh_lpm_ctx.status) {
                int ret = 0;
                //LOGD(TAG,"[%u]wake up and resume mesh stack\n", aos_now_ms());
                ret = bt_mesh_resume_lpm(IS_RESUME_SCAN(mesh_lpm_ctx.p_config.lpm_mode));
                if (0 == ret) {
                    mesh_lpm_ctx.status = STATUS_WAKEUP;
                    _mesh_lpm_io_status_list_t list;
                    list.size = g_wakeup_io_num;
                    list.io_status = g_io_status;
                    mesh_lpm_ctx.p_config.mesh_lpm_cb(WAKEUP_BY_IO, mesh_lpm_ctx.status, &list);
                }
            } else {
                mesh_lpm_ctx.p_config.mesh_lpm_cb(WAKEUP_IS_WAKEUP, mesh_lpm_ctx.status, NULL); //TODO check if can enter
            }
        }
    }
}

#if defined(CONFIG_PM_SLEEP) && CONFIG_PM_SLEEP > 0
static void _mesh_lpm_io_wakeup_init(mesh_lpm_wakeup_io_t config)
{
    if (config.io_list_size > MESH_WAKE_UP_IO_NUM_MAX) {
        LOGE(TAG,"Wakeup i/o num should no more than %d", MESH_WAKE_UP_IO_NUM_MAX);
    }
    g_wakeup_io_num = MESH_WAKE_UP_IO_NUM_MAX < config.io_list_size ? MESH_WAKE_UP_IO_NUM_MAX : config.io_list_size;
    memcpy(g_wakeup_io, config.io_config, sizeof(mesh_lpm_wakeup_io_config_t) * g_wakeup_io_num);

    for (int i = 0; i < g_wakeup_io_num; i++) {
        mesh_sal_sleep_wakup_io_set(g_wakeup_io[i].port, g_wakeup_io[i].io_pol);
    }

    mesh_sal_io_wakeup_cb_register(mesh_lpm_io_wakeup_handler);
}
#endif

int mesh_lpm_disable(void)
{
    LOGI(TAG,"lpm disable");

    if(!mesh_lpm_ctx.is_mesh_lpm_start) {
        LOGE(TAG,"mesh lpm not start");
        return -1;
    }

    if (mesh_lpm_ctx.is_mesh_init == 1) {
        bt_mesh_resume_lpm(true);
    }

    mesh_lpm_ctx.is_mesh_lpm_disabled = 1;
    mesh_lpm_ctx.status = STATUS_WAKEUP;
    aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
    mesh_sal_sleep_disable();
    return 0;
}

int mesh_lpm_enable(bool force)
{
    LOGI(TAG,"lpm enable");
    if(!mesh_lpm_ctx.is_mesh_lpm_start) {
        LOGE(TAG,"mesh lpm not start");
        return -1;
    }

    if (mesh_lpm_ctx.is_mesh_init == 1) {
        bt_mesh_suspend_lpm(force);
    }

    mesh_lpm_ctx.is_mesh_lpm_disabled = 0;
    mesh_sal_sleep_enable();
    mesh_lpm_ctx.status = STATUS_SLEEP;

    if (force) {
        aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
    } else {
        aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
        aos_timer_change(&mesh_lpm_ctx.wakeup_timer, mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.sleep_ms);
        aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
    }

    return 0;
}




int mesh_lpm_deep_sleep(void)
{
    *(volatile uint32_t *)PWR_BOOT_REASON = PWR_STANDBY_BOOT_FLAG;
    return mesh_sal_sleep_enter_standby();
}


int mesh_lpm_init(mesh_lpm_conf_t *lpm_conf)
{
#ifndef CONFIG_PM_SLEEP
    LOGE(TAG,"enable the chip pm first");
    return -ENOTSUP;
#else

    uint32_t *p_boot_reason = (uint32_t *)PWR_BOOT_REASON;

    memcpy(&mesh_lpm_ctx.p_config, lpm_conf, sizeof(mesh_lpm_conf_t));

    if (mesh_lpm_ctx.p_config.mesh_lpm_cb == NULL) {
        LOGE(TAG,"lpm cb is null");
        return -EINVAL;
    }

    if(mesh_lpm_ctx.p_config.lpm_wakeup_mode != WAKEUP_BY_IO_MODE && (!mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.sleep_ms ||    \
            !mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.wakeup_ms)) {
        LOGE(TAG,"valid wakeup timer config");
        return -EINVAL;
    }

    if(mesh_lpm_ctx.p_config.lpm_wakeup_mode != WAKEUP_BY_TIMER_MODE && (!mesh_lpm_ctx.p_config.lpm_wakeup_io_config.io_list_size || \
            !mesh_lpm_ctx.p_config.lpm_wakeup_io_config.io_config)) {
        LOGE(TAG,"valid wakeup io config");
        return -EINVAL;
    }

    if (mesh_lpm_ctx.p_config.delay_sleep_time == 0) {
        mesh_lpm_ctx.p_config.delay_sleep_time = DEFAULT_BOOTUP_DELAY_SLEEP_TIME;
    }

    mesh_lpm_ctx.status = STATUS_WAKEUP;

    LOGI(TAG,"lpm mode: %02x wakeup mode:%02x",mesh_lpm_ctx.p_config.lpm_mode, mesh_lpm_ctx.p_config.lpm_wakeup_mode);
    if(mesh_lpm_ctx.p_config.lpm_wakeup_mode != WAKEUP_BY_IO_MODE) {
        LOGI(TAG,"sleep:%d ms, wakeup:%d ms",mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.sleep_ms,mesh_lpm_ctx.p_config.lpm_wakeup_timer_config.wakeup_ms);
    }

    if (mesh_lpm_ctx.p_config.lpm_wakeup_mode != WAKEUP_BY_TIMER_MODE) { //mode IO nad mode IO_TIMER
        _mesh_lpm_io_wakeup_init(lpm_conf->lpm_wakeup_io_config);
    }

#if !(CONFIG_MESH_ENABLE_SLEEP_WHEN_NOT_PROV)
    mesh_sal_sleep_disable();
#else
    mesh_sal_sleep_enable();
#endif

    if (*p_boot_reason == PWR_STANDBY_BOOT_FLAG) {
        LOGI(TAG,"standy boot flag");
        *(volatile uint32_t *)PWR_BOOT_REASON = 0;
    }

    aos_timer_new_ext(&mesh_lpm_ctx.wakeup_timer, _mesh_lpm_timer_irq_handler, NULL, mesh_lpm_ctx.p_config.delay_sleep_time, 0, 0);

    mesh_lpm_ctx.is_mesh_lpm_init = 1;

    return 0;
#endif
}



int mesh_lpm_start(void)
{
    if(!mesh_lpm_ctx.is_mesh_lpm_init) {
        LOGE(TAG,"mesh lpm not init");
        return -1;
    }

    if(mesh_lpm_ctx.is_mesh_lpm_start) {
        return -EALREADY;
    }

    mesh_lpm_ctx.is_mesh_init = 1;
    LOGD(TAG,"mesh lpm mesh ready");
    if (mesh_lpm_ctx.p_config.is_auto_enable == 1) {
        mesh_lpm_ctx.is_mesh_lpm_disabled = 0;
        //mesh_sal_sleep_enable();
        mesh_lpm_ctx.status = STATUS_WAKEUP;
        aos_timer_stop(&mesh_lpm_ctx.wakeup_timer);
        aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
    } else {
        if (mesh_lpm_ctx.p_config.delay_sleep_time > 0) {
            mesh_lpm_ctx.is_mesh_lpm_disabled = 0;
            aos_timer_start(&mesh_lpm_ctx.wakeup_timer);
        }
    }
    LOGI(TAG,"the dev will enter lpm in %d ms",mesh_lpm_ctx.p_config.delay_sleep_time);
    mesh_lpm_ctx.is_mesh_lpm_start = 1;

    return 0;
}

void mesh_lpm_tx_handle()
{
	if(!mesh_lpm_ctx.is_mesh_lpm_start) {
      return;
	}

	if(mesh_lpm_ctx.p_config.lpm_mode == MESH_LPM_MODE_TX_RX) {
        bt_mesh_scan_enable();
	}
}

#endif
