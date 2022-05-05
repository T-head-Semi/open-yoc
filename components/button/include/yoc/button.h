/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __BUTTON__
#define __BUTTON__

#include <drv/gpio.h>

typedef struct button button_t;

typedef enum {
    BUTTON_PRESS_DOWN = 0,
    BUTTON_PRESS_UP,
    BUTTON_PRESS_LONG_DOWN,
    BUTTON_PRESS_DOUBLE,
    BUTTON_PRESS_TRIPLE
} button_evt_id_t;

typedef enum {
    BUTTON_TYPE_GPIO,/* GPIO按键 支持中断方式*/
    BUTTON_TYPE_ADC, /* ADC按键 */
} button_type_t;

typedef enum {
    LOW_LEVEL = 0,
    HIGH_LEVEL
} button_gpio_level_t;

typedef void (*button_evt_cb_t)(int evt_id, void *priv);

typedef struct {
    int             button_id;  //按键id
    button_evt_id_t event_id;   //按键事件
    int             press_time; //当按键事件为长按时，为长按事件
} button_evt_t;

/**
 * @brief  button init
 * @return 0 on success, -1 on failed
 */
int button_init(void);

/**
 * @brief  button deinit
 * @return 0 on success, -1 on failed
 */
int button_deinit(void);

/**
 * @brief  button table init
 * @param  [in] button_id        : button id
 * @param  [in] gpio_pin         : gpio pin
 * @param  [in] active_level     : active level
 * @return 0 on success, -1 on failed
 */
int button_add_gpio(int button_id, int gpio_pin, button_gpio_level_t active_level);

/**
 * @brief  add adc button
 * @param  [in] button_id        : button id
 * @param  [in] adc_name         : adc device name
 * @param  [in] adc_channel      : adc channel
 * @param  [in] vol_ref          : adc voltage reference
 * @param  [in] vol_range        : adc voltage range
 * @return 0 on success, -1 on failed
 */
int button_add_adc(int button_id, char *adc_name, int adc_channel, int vol_ref, int vol_range);

/**
 * @brief  button add event
 * @param  [in] evt_id           : user event id
 * @param  [in] buttons          : button table
 * @param  [in] button_count     : button count
 * @param  [in] evt_cb           : event callback
 * @param  [in] priv             : user private
 * @return 0 on success, -1 on failed
 */
int button_add_event(int evt_id, button_evt_t *buttons, int button_count, button_evt_cb_t evt_cb, void *priv);

/**
 * @brief  check if button is pressed
 * @param  [in] button_id        : button_id
 * @param  [in] pressed          : press flag
 * @return 0 on success, -1 on failed
 */
int button_is_pressed(int button_id, bool *pressed);

#endif
