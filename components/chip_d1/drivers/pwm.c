#include <drv/pwm.h>
#include <drv/irq.h>

#include <sunxi_hal_pwm.h>

#define PWM_CHANNEL_MAX_NUM (8)

typedef struct {
    struct pwm_config config[PWM_CHANNEL_MAX_NUM];
    int channel;
} d1_pwm_t;

volatile int g_pwn_inited;
struct pwm_config g_pwm_config[PWM_CHANNEL_MAX_NUM];

csi_error_t csi_pwm_init(csi_pwm_t *pwm, uint32_t idx)
{
    if (idx != 0) {
        return CSI_ERROR;
    }

    if (!g_pwn_inited) {
        aw_hal_pwm_init();
    }

    g_pwn_inited ++;
    pwm->dev.idx = idx;

    return 0;
}

void csi_pwm_uninit(csi_pwm_t *pwm)
{
    if (!(--g_pwn_inited)) {
        hal_pwm_deinit();
    }
}

csi_error_t csi_pwm_out_config(csi_pwm_t *pwm,
                               uint32_t  channel,
                               uint32_t period_us,
                               uint32_t pulse_width_us,
                               csi_pwm_polarity_t polarity)
{
    if (channel > (PWM_CHANNEL_MAX_NUM - 1)) {
        return CSI_ERROR;
    }

    g_pwm_config[channel].duty_ns = pulse_width_us * 1000;
    g_pwm_config[channel].period_ns = period_us * 1000;
    g_pwm_config[channel].polarity = polarity;

    return 0;
}

csi_error_t csi_pwm_out_start(csi_pwm_t *pwm, uint32_t channel)
{
    hal_pwm_control(channel, &g_pwm_config[channel]);

    return 0;
}

void csi_pwm_out_stop(csi_pwm_t *pwm, uint32_t channel)
{
    if (channel > (PWM_CHANNEL_MAX_NUM - 1)) {
        return;
    }
    hal_pwm_disable_controller(channel);
}

