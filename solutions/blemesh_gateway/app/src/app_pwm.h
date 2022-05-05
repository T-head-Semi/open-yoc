/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _APP_PWM_H_
#define _APP_PWM_H_

/**
 * PWM Initialize
 * @param [in] pwm_pin : PWM pin name
 * @param [in] pin_func: PWM pin function
 * @param [in] ch      : PWM channel number
 * @return 0 on success, negative value on failed
 */
int8_t app_pwm_init(int pwm_pin, int pin_func, uint8_t ch);

/**
 * PWM Deinitialize
 * @return 0 on success, negative value on failed
 */
int8_t app_pwm_deinit(void);

/**
 * start PWM
 * @param [in] ch      : PWM channel number
 * @return 0 on success, negative value on failed
 */
int8_t app_pwm_start(uint8_t ch);

/**
 * stop PWM
 * @param [in] ch      : PWM channel number
 * @return 0 on success, negative value on failed
 */
int8_t app_pwm_stop(uint8_t ch);

/**
 * config PWM
 * @param [in] ch      : PWM channel number
 * @param [in] period  : PWM period (us)
 * @param [in] duty    : PWM duty (0 ~ 100)
 * @return 0 on success, negative value on failed
 */
int8_t app_pwm_config(uint8_t ch, uint32_t period, uint32_t duty);

#endif