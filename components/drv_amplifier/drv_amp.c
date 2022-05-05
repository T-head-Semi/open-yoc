
#include <stdio.h>
#include <stdlib.h>
#include <drv/gpio.h>
#ifdef CONFIG_CSI_V1
#include <pinmux.h>
#endif
#include <aos/kernel.h>
#include <ulog/ulog.h>

#include "drv_amp.h"
#include "drv_amp_ops.h"

#define TAG "PA"

static amplifier_pa_priv_t g_pa_priv = {0, 0, 0, 0, NULL, NULL, -1};

extern const struct amplifier_pa_ops  g_padef_ops;
extern const struct amplifier_pa_ops  g_mix2910_ops;
extern const struct amplifier_pa_ops  g_aw87519_ops;
extern const struct amplifier_pa_ops  g_cs8122s_ops;
static const struct amplifier_pa_ops *pa_ops[APM_SUPPORT_COUNT];


static void gpio_init(int pin_ctrl, int pin_pwr)
{

    g_pa_priv.pa_ctrl_pin  = pin_ctrl;
    g_pa_priv.pa_power_pin = pin_pwr;

#ifdef CONFIG_CSI_V1
    if (pin_ctrl > 0) {
        drv_pinmux_config(pin_ctrl, PIN_FUNC_GPIO);
        g_pa_priv.pa_mute_hdl = csi_gpio_pin_initialize(pin_ctrl, NULL);
        csi_gpio_pin_config_mode(g_pa_priv.pa_mute_hdl, GPIO_MODE_PULLNONE);
        csi_gpio_pin_config_direction(g_pa_priv.pa_mute_hdl, GPIO_DIRECTION_OUTPUT);
    }

    if (pin_pwr > 0) {
        drv_pinmux_config(pin_pwr, PIN_FUNC_GPIO);
        g_pa_priv.pa_power_hdl = csi_gpio_pin_initialize(pin_pwr, NULL);
        csi_gpio_pin_config_mode(g_pa_priv.pa_power_hdl, GPIO_MODE_PULLNONE);
        csi_gpio_pin_config_direction(g_pa_priv.pa_power_hdl, GPIO_DIRECTION_OUTPUT);
        csi_gpio_pin_write(g_pa_priv.pa_power_hdl, true);
    }
#endif
}


int amplifier_init(int amp_type, int mute_pin, int power_pin, int amp_mode)
{
    if (amp_type < 0 || amp_type >= APM_SUPPORT_COUNT) {
        return -1;
    }

    if (amp_mode < 0) {
        return -1;
    }

    for (int i = 0; i < APM_SUPPORT_COUNT; i++) {
        pa_ops[i] = NULL;
    }

    pa_ops[0] = &g_padef_ops;
    pa_ops[1] = &g_mix2910_ops;
    pa_ops[2] = &g_aw87519_ops;
    pa_ops[3] = &g_cs8122s_ops;

    gpio_init(mute_pin, power_pin);

    /* probe active model */
    g_pa_priv.pa_config    = amp_mode;
    g_pa_priv.active_pa_id = amp_type;

    pa_ops[g_pa_priv.active_pa_id]->init(&g_pa_priv);
    LOGD(TAG, "Probe %s", pa_ops[g_pa_priv.active_pa_id]->name);

    return 0;
}

int amplifier_deinit()
{
    if (g_pa_priv.active_pa_id < 0) {
        return -1;
    }

    if (pa_ops[g_pa_priv.active_pa_id] == NULL) {
        return 0;
    }

    pa_ops[g_pa_priv.active_pa_id]->uninit(&g_pa_priv);

#ifdef CONFIG_CSI_V1
    if (g_pa_priv.pa_mute_hdl) {
        csi_gpio_pin_write(g_pa_priv.pa_mute_hdl, false);
    }

    if (g_pa_priv.pa_power_hdl) {
        csi_gpio_pin_write(g_pa_priv.pa_power_hdl, false);
    }
#endif
    return 0;
}

int amplifier_onoff(int onoff)
{
    if (g_pa_priv.active_pa_id < 0) {
        return -1;
    }

    int amp_mode     = g_pa_priv.pa_config & 0x0000FFFF;
    int pa_lock_open = g_pa_priv.pa_config & 0xFFFF0000;

    /* 状态相同不处理 */
    if (g_pa_priv.pa_onoff == onoff) {
        return 0;
    }

    /* 常开模式，忽略关闭命令 */
    if (onoff == 0 && pa_lock_open) {
        return 0;
    }

    g_pa_priv.pa_onoff = onoff;

    return pa_ops[g_pa_priv.active_pa_id]->onoff(&g_pa_priv, onoff, amp_mode);
}

int amplifier_getid(void)
{
    if (g_pa_priv.active_pa_id < 0) {
        return -1;
    }

    if (pa_ops[g_pa_priv.active_pa_id]->getid == NULL) {
        return -1;
    }

    return pa_ops[g_pa_priv.active_pa_id]->getid();
}

int amplifier_reset(void)
{
    if (g_pa_priv.active_pa_id < 0) {
        return -1;
    }

    if (pa_ops[g_pa_priv.active_pa_id]->reset == NULL) {
        return -1;
    }

    return pa_ops[g_pa_priv.active_pa_id]->reset(&g_pa_priv);
}

int amplifier_config(int amp_mode, int lock_on)
{
    if (amp_mode >= 0) {
        g_pa_priv.pa_config = amp_mode;
    }

    if (lock_on) {
        g_pa_priv.pa_config |= AMP_MODE_LOCK_ON;
    }

    g_pa_priv.pa_onoff = 0; /* 设置未关闭,下次开启PA重新配置 */

    return 0;
}

int amplifier_cfgbin_read(uint8_t *byte, uint16_t count)
{
    if (g_pa_priv.active_pa_id < 0) {
        return -1;
    }

    if (pa_ops[g_pa_priv.active_pa_id]->cfgbin_read) {
        return pa_ops[g_pa_priv.active_pa_id]->cfgbin_read(byte, count);
    }

    return -1;
}

int amplifier_cfgbin_write(uint8_t *byte, uint16_t count)
{
    if (g_pa_priv.active_pa_id < 0) {
        return -1;
    }

    if (pa_ops[g_pa_priv.active_pa_id]->cfgbin_write) {
        return pa_ops[g_pa_priv.active_pa_id]->cfgbin_write(byte, count);
    }

    return -1;
}
