
#include <aos/kernel.h>

#include <drv/gpio.h>
#include <pinmux.h>

#include "drv_amp.h"
#include "drv_amp_ops.h"

#define PA_EN_DELAY 150 /*ms*/

extern void udelay(uint32_t us);

static int amplifier_onoff_mix2910(gpio_pin_handle_t handle, int onoff, int amp_mode)
{
    uint32_t irq_flag = 0;

    /* 关闭 PA */
    if (onoff == 0) {
        csi_gpio_pin_write(handle, 0);
        return 0;
    }

    /* 普通拉高使能模式 */
    switch (amp_mode) {
    case AMP_MODE_D1:
        csi_gpio_pin_write(handle, 1);
        aos_msleep(PA_EN_DELAY);
        break;
    case AMP_MODE_D2:
        csi_gpio_pin_write(handle, 0);
        aos_msleep(10);

        csi_gpio_pin_write(handle, 1);
        aos_msleep(20);

        irq_flag = csi_irq_save();

        csi_gpio_pin_write(handle, 0);
        udelay(70);
        csi_gpio_pin_write(handle, 1);
        udelay(70);
        csi_gpio_pin_write(handle, 0);
        udelay(70);
        csi_gpio_pin_write(handle, 1);

        csi_irq_restore(irq_flag);
        /* wait PA ready */
        aos_msleep(PA_EN_DELAY - 30);
        break;
    case AMP_MODE_D3:
        csi_gpio_pin_write(handle, 0);
        aos_msleep(10);

        csi_gpio_pin_write(handle, 1);
        aos_msleep(20);

        irq_flag = csi_irq_save();

        csi_gpio_pin_write(handle, 0);
        udelay(70);
        csi_gpio_pin_write(handle, 1);
        udelay(70);
        csi_gpio_pin_write(handle, 0);
        udelay(70);
        csi_gpio_pin_write(handle, 1);
        udelay(70);
        csi_gpio_pin_write(handle, 0);
        udelay(70);
        csi_gpio_pin_write(handle, 1);

        csi_irq_restore(irq_flag);
        /* wait PA ready */
        aos_msleep(PA_EN_DELAY - 30);
        break;
    default:;
    }

    return 0;
}

static int mix_amplifier_init(amplifier_pa_priv_t *priv)
{
    return 0;
}

static int mix_amplifier_uninit(amplifier_pa_priv_t *priv)
{
    return 0;
}

static int mix_amplifier_onoff(amplifier_pa_priv_t *priv, int onoff, int amp_mode)
{
    return amplifier_onoff_mix2910(priv->pa_mute_hdl, onoff, amp_mode);
}

const struct amplifier_pa_ops g_mix2910_ops = {.name         = "mix2910",
                                               .init         = mix_amplifier_init,
                                               .uninit       = mix_amplifier_uninit,
                                               .onoff        = mix_amplifier_onoff,
                                               .probe        = NULL,
                                               .cfgbin_read  = NULL,
                                               .cfgbin_write = NULL,
                                               .getid        = NULL,
                                               .reset        = NULL};
