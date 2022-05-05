/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef BUTTON_INTERNAL_H
#define BUTTON_INTERNAL_H

#include <aos/aos.h>
#include <aos/list.h>
#include <aos/hal/gpio.h>


#define ADC_NAME_MAX (10)
typedef struct button_ops {
    int (*init)(button_t *b);
    int (*deinit)(button_t *b);
    int (*read)(button_t *b);
    int (*irq_disable)(button_t *b);
    int (*irq_enable)(button_t *b);
} button_ops_t;

typedef struct gpio_button_param {
    int                 pin_id;
    gpio_dev_t         *pin_hdl;
    button_gpio_level_t active_level;
} gpio_button_param_t;

typedef struct adc_button_param {
    char    adc_name[ADC_NAME_MAX];
    int     channel;
    int     range;
    int     vref;
} adc_button_param_t;

struct button {
    slist_t         next;
    int             repeat;
    int             state;
    int             old_evt_id;
    int             active;
    int             irq_flag;
    bool            is_pressed;
    int             button_id;
    int             type;
    int            *press_time;
    int             press_time_subscript;
    int             press_time_cnt;
    int             event_id;
    int             event_flag;
    long long       happened_ms; //event happen tick
    long long       st_ms;    //enter irq time
    button_ops_t   *ops;
    void           *param;
};

typedef button_evt_t event_pool_elem_t;

typedef struct event_node {
    slist_t             next;
    int                 event_id;
    int                 start_time;
    button_evt_t       *buttons;
    int                 button_count;
    int                 event_depth;
    int                 pending;
    button_evt_cb_t     evt_cb;
    void               *priv;
} event_node_t;

typedef struct button_srv {
    aos_timer_t         tmr;
    int                 start_tmr;
    int                 adc_flag;
    int                 inited;
    slist_t             button_head;
    slist_t             event_node_head;
    int                 event_pool_depth;
    event_pool_elem_t  *event_pool;
    int                 event_pool_size;
    long long           happened_ms; //event last happen tick
} button_srv_t;

extern button_ops_t gpio_ops;
extern button_ops_t adc_ops;
extern button_srv_t g_button_srv;
void button_irq(button_t *button);

#endif
