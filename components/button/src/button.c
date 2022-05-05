/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <yoc/button.h>

#ifdef CONFIG_CSI_V2
#include <soc.h>
#else
#include <pinmux.h>
#endif

#include "internal.h"

#define MIN_ST_TMOUT (60)
#define MAX_EVENT_TMOUT (500)

#define BUTTON_SCAN_TIME (20)


#define BUTTON_OPS(b, fn) \
    if (b->ops->fn) \
        b->ops->fn(b)

#define TAG "button"

static event_node_t *event_find(event_pool_elem_t *elem);
static int event_manage(void);
static int event_pool_clean(event_node_t *e_node);
static button_t *button_find(int button_id);

typedef enum {
    NONE,
    START,
    CNT,
    HIGH,
    END,
} button_state;

button_srv_t g_button_srv;

static void irq_disable(button_t *button)
{
    BUTTON_OPS(button, irq_disable);
}

static void irq_enable(button_t *button)
{
    BUTTON_OPS(button, irq_enable);
}

void button_irq(button_t *button)
{
    irq_disable(button);
    button->irq_flag = 1;
    button->st_ms = aos_now_ms();

    if (aos_timer_is_valid(&g_button_srv.tmr)) {
        aos_timer_start(&g_button_srv.tmr);
    } else {
        irq_enable(button);
    }
}

static int reset(button_t *button)
{
    button->state = NONE;
    button->active = 0;
    button->press_time_subscript = 0;

    return 0;
}

static int none_hdl(button_t *button)
{
    if (button->is_pressed) {
        button->state = START;
        g_button_srv.start_tmr = 1;
    } else {
        irq_enable(button);
    }

    return 0;
}

static int start_hdl(button_t *button)
{
    if (button->is_pressed) {
        if ((aos_now_ms() - button->st_ms) >= MIN_ST_TMOUT) {
            // button->active = 1;
            button->state = CNT;
        }

        g_button_srv.start_tmr = 1;
    } else {
        reset(button);
        irq_enable(button);
    }

    return 0;
}

static int cnt_hdl(button_t *button)
{
    if (button->is_pressed && button->press_time_cnt) {
        if ((aos_now_ms() - button->st_ms) >= button->press_time[button->press_time_subscript]) {
            button->active = 1;
            button->event_id = BUTTON_PRESS_LONG_DOWN;
            button->press_time_subscript ++;

            if (button->press_time_subscript >= button->press_time_cnt) {
                button->state = HIGH;
            }
        }

        g_button_srv.start_tmr = 1;
    } else {
        button->state = HIGH;
        button->active = 1;
        button->event_id = BUTTON_PRESS_DOWN;
        g_button_srv.start_tmr = 1;
        button->old_evt_id = BUTTON_PRESS_DOWN;
    }

    return 0;
}

static int high_hdl(button_t *button)
{
    if (!button->is_pressed) {
        button->state = END;
        button->active = 1;

        if (button->event_id == BUTTON_PRESS_LONG_DOWN) {
            button->event_id = -1;
        } else {
            button->event_id = BUTTON_PRESS_UP;
        }

        g_button_srv.start_tmr = 1;
        // irq_enable(button);
    } else {
        g_button_srv.start_tmr = 1;
    }

    return 0;
}

static int read_level(button_t *button)
{
    if (button->ops->read) {
        return (button->ops->read(button));
    }

    return -1;
}

/*
    ----|
*/
static int _button_hdl(button_t *button)
{
    button->is_pressed = read_level(button);

    // LOGD(TAG, "is_pressed:%d state:%d",  button->is_pressed, button->state);

    switch (button->state) {
        case NONE:
            none_hdl(button);
            break;

        case START:
            start_hdl(button);
            break;

        case CNT:
            cnt_hdl(button);
            break;

        case HIGH:
            high_hdl(button);
            break;

        default:
            reset(button);
            irq_enable(button);
            break;
    }

    // LOGD(TAG, "state:%d, name:%d active:%d evt:%d", button->state, button->button_id, button->active, button->event_id);

    if (button->active) {
        if ((button->event_flag & (1 << button->event_id)) == 0) {
            button->active = 0;
        }
    }

    if (button->active) {
        button->happened_ms = aos_now_ms();
    }

    return 0;
}

static void button_adc_sttime_check(void)
{
    button_t *b = NULL;

    slist_for_each_entry(&g_button_srv.button_head, b, button_t, next) {
        button_ops_t *ops = &adc_ops;

        if ((b->ops == ops) && b->irq_flag == 0 && b->st_ms == 0) {
            bool is_pressed = b->ops->read(b);

            if (is_pressed) {
                b->is_pressed = true;
                b->st_ms = aos_now_ms();
                b->irq_flag = 1;
            }
        }
    }
}

static int event_pool_add(button_t *b)
{
    if (g_button_srv.event_pool_depth >= g_button_srv.event_pool_size) {
        return -1;
    }

    event_pool_elem_t *event_elem = &g_button_srv.event_pool[g_button_srv.event_pool_depth];

    event_elem->button_id  = b->button_id;
    event_elem->event_id   = b->event_id;

    if (b->press_time_subscript) {
        event_elem->press_time = b->press_time[b->press_time_subscript - 1];
    } else {
        event_elem->press_time = 0;
    }

    g_button_srv.event_pool_depth ++;
    b->active = 0;

    return 0;
}

static int event_param_check(button_evt_t *buttons, int button_count)
{
    for (int i = 0; i < button_count; i ++) {
        if (button_find(buttons[i].button_id) == NULL) {

            return -1;
        }
    }

    event_node_t *e_node;
    slist_for_each_entry(&g_button_srv.event_node_head, e_node, event_node_t, next) {
        if ((button_count == e_node->button_count) && (memcmp(e_node->buttons, buttons, sizeof(button_evt_t)*button_count) == 0)) {
            return -1;         
        }
    }

    return 0;
}

static button_t *button_find(int button_id)
{
    button_t *b = NULL;

    slist_for_each_entry(&g_button_srv.button_head, b, button_t, next) {
        if (b->button_id == button_id) {
            break;
        }
    }

    return b;
}

static int event_pool_reset(void)
{
    event_node_t *node;
    event_node_t *e_sub = NULL;

    int cnt = 0;
    slist_for_each_entry(&g_button_srv.event_node_head, node, event_node_t, next) {
        if (node->button_count == g_button_srv.event_pool_depth) {
            int ret = memcmp(node->buttons, g_button_srv.event_pool, \
                             sizeof(event_pool_elem_t) * node->button_count);

            if (ret == 0) {
                node->event_depth = node->button_count;

                if (node->button_count > cnt) {
                    e_sub = node;
                }
            }
        }
    }

    if (e_sub) {
        if (e_sub->event_depth == e_sub->button_count) {
            e_sub->evt_cb(e_sub->event_id, e_sub->priv);
            event_pool_clean(e_sub);
        }
    }

    if (e_sub == NULL && g_button_srv.event_pool_depth) {

        for (int i = 0; i < g_button_srv.event_pool_depth; i++) {
            event_pool_elem_t *event_elem = &g_button_srv.event_pool[i];
            event_node_t *e_node = event_find(event_elem);

            if (e_node) {
                e_node->evt_cb(e_node->event_id, e_node->priv);
            }
        }
    }
    event_pool_clean(NULL);

    return 0;
}

static int event_pool_clean(event_node_t *e_node)
{
    if (e_node == NULL) {
        g_button_srv.event_pool_depth = 0;
    } else {
        int depth = g_button_srv.event_pool_depth - e_node->button_count;

        g_button_srv.event_pool_depth = depth;
        memcpy(g_button_srv.event_pool, &g_button_srv.event_pool[e_node->button_count], depth*sizeof(button_evt_t));
    }
    return 0;
}

static int event_manage(void)
{
    event_node_t *node;
    event_node_t *e_sub = NULL;

    int cnt = 0;
    slist_for_each_entry(&g_button_srv.event_node_head, node, event_node_t, next) {
        if (node->button_count >= g_button_srv.event_pool_depth) {
            int ret = memcmp(node->buttons, g_button_srv.event_pool, \
                             sizeof(event_pool_elem_t) * g_button_srv.event_pool_depth);

            if (ret == 0) {
                node->event_depth = g_button_srv.event_pool_depth;

                if (node->button_count > cnt) {
                    e_sub = node;
                }
            }
        }
    }

    if (e_sub) {
        if (e_sub->event_depth == e_sub->button_count) {
            e_sub->evt_cb(e_sub->event_id, e_sub->priv);
            event_pool_clean(e_sub);
        }
    }

    if (e_sub == NULL && g_button_srv.event_pool_depth) {
        slist_for_each_entry(&g_button_srv.event_node_head, node, event_node_t, next) {
            if (node->button_count == (g_button_srv.event_pool_depth - 1)) {
                int ret = memcmp(node->buttons, g_button_srv.event_pool, \
                                 sizeof(event_pool_elem_t) * (g_button_srv.event_pool_depth - 1));

                if (ret == 0) {
                    node->evt_cb(node->event_id, node->priv);
                    event_pool_clean(node);
                    break;
                }
            }
        }
    }

    return 0;
}

static void button_timer_entry(void *timer, void *arg)
{
    button_t *b = NULL;
    int cnt = 0;

    g_button_srv.start_tmr = 0;
    aos_timer_stop(&g_button_srv.tmr);

    if (g_button_srv.event_pool_depth) {
        if ((aos_now_ms() - g_button_srv.happened_ms) >= MAX_EVENT_TMOUT) {
            event_pool_reset();
        }
    }

    button_adc_sttime_check();

    slist_for_each_entry(&g_button_srv.button_head, b, button_t, next) {
        if (b->irq_flag == 1 || b->state > 0) {
            b->irq_flag = 0;
            _button_hdl(b);

            if (b->active == 1) {
                if ((g_button_srv.happened_ms == 0) || ((b->happened_ms - g_button_srv.happened_ms) < MAX_EVENT_TMOUT)) {
                    g_button_srv.happened_ms = b->happened_ms;
                    if (event_pool_add(b) < 0) {
                        event_pool_reset();
                    }
                    event_manage();
                } else {
                    g_button_srv.happened_ms = b->happened_ms;
                    event_pool_reset();
                    event_pool_add(b);
                    event_manage();
                }

                if (g_button_srv.event_pool_depth == 0) {
                    g_button_srv.happened_ms = 0;
                }
            }
            cnt ++;
        }
    }

    // LOGE(TAG, "timer is %s\n", g_button_srv.start_tmr? "continue" : "stop");
    if (g_button_srv.adc_flag || g_button_srv.start_tmr || g_button_srv.event_pool_depth) {
        aos_timer_start(&g_button_srv.tmr);
    }
}

static int button_set_ops(button_t *button, button_ops_t *ops)
{
    button->ops = ops;

    return 0;
}

static int button_new(button_t **b, int type)
{
    *b = aos_zalloc_check(sizeof(button_t));

    if (type == BUTTON_TYPE_GPIO) {
        (*b)->param = aos_zalloc_check(sizeof(gpio_button_param_t));
    } else if (type == BUTTON_TYPE_ADC) {
        (*b)->param = aos_zalloc_check(sizeof(adc_button_param_t));
    } else {
        aos_free(*b);
        *b = NULL;
    }

    if (*b) {
        (*b)->event_id = -1;
    }

    return *b == NULL ? -1 : 0;
}

static int button_destroy(button_t *b)
{
    if (b->press_time) {
        aos_free(b->press_time);
    }
    aos_free(b->param);
    aos_free(b);
    return 0;
}

static int button_add(button_t *b)
{
    slist_add_tail(&b->next, &g_button_srv.button_head);

    return 0;
}

static int button_remove(button_t *b)
{
    slist_del(&b->next, &g_button_srv.button_head);

    return 0;
}

static int event_new(event_node_t **e_node, int button_cnt)
{
    *e_node = aos_zalloc_check(sizeof(event_node_t));

    (*e_node)->buttons = aos_zalloc_check(sizeof(button_evt_t) * button_cnt);

    return 0;
}

static int event_destory(event_node_t *e_node)
{
    aos_free(e_node->buttons);
    aos_free(e_node);

    return 0;
}

static int event_add(event_node_t *e_node)
{
    slist_add_tail(&e_node->next, &g_button_srv.event_node_head);

    return 0;
}

static int buttton_check(int button_id)
{
    button_t *b = NULL;

    b = button_find(button_id);

    return b == NULL? 0 : -1;
}

static int event_remove(event_node_t *e_node)
{
    slist_del(&e_node->next, &g_button_srv.event_node_head);

    return 0;
}

static event_node_t *event_find(event_pool_elem_t *elem)
{
    event_node_t *e_node;

    slist_for_each_entry(&g_button_srv.event_node_head, e_node, event_node_t, next) {
        if ((e_node->button_count == 1) && (0 == memcmp(elem, e_node->buttons, sizeof(event_pool_elem_t)))) {
            return e_node;
        }
    }

    return NULL;
}

static int event_check(int evt_id)
{
    event_node_t *e_node;

    slist_for_each_entry(&g_button_srv.event_node_head, e_node, event_node_t, next) {
        if (e_node->event_id == evt_id) {
            return -1;
        }
    }

    return 0;
}

static int event_buttons_count(button_evt_t *buttons, int button_conut)
{
    int cnt = 0;

    for (int i = 0; i < button_conut; i++) {
        if (buttons[i].event_id == BUTTON_PRESS_DOUBLE || buttons[i].event_id == BUTTON_PRESS_TRIPLE) {
            cnt += ((buttons[i].event_id - 1) * 2);
        } else {
            cnt ++;
        }
    }

    return cnt;
}

static int event_copy(event_node_t *e_node, button_evt_t *buttons, int button_conut)
{
    button_evt_t *b_dec = e_node->buttons;
    int offset = 0;;

    for (int i = 0; i < button_conut; i++) {
        if (buttons[i].event_id == BUTTON_PRESS_DOUBLE || buttons[i].event_id == BUTTON_PRESS_TRIPLE) {
            int count = buttons[i].event_id - 1;

            for (int j = 0; j < count; j++) {
                b_dec[offset + j * 2].event_id    =  BUTTON_PRESS_DOWN;
                b_dec[offset + j * 2].button_id   = buttons[i].button_id;
                b_dec[offset + j * 2 + 1].event_id  =  BUTTON_PRESS_UP;
                b_dec[offset + j * 2 + 1].button_id = buttons[i].button_id;
            }

            offset += (count * 2);
        } else {
            memcpy(&b_dec[offset], &buttons[i], sizeof(button_evt_t));

            if (buttons[i].event_id != BUTTON_PRESS_LONG_DOWN) {
                b_dec[offset].press_time = 0;
            }

            offset ++;
        }
    }

    return 0;
}

static int event_pool_new(int event_pool_size)
{
    if (event_pool_size > g_button_srv.event_pool_size) {
        g_button_srv.event_pool      = aos_realloc_check(g_button_srv.event_pool, sizeof(button_evt_t) * event_pool_size);
        g_button_srv.event_pool_size = event_pool_size;
        memset(g_button_srv.event_pool, 0x00, sizeof(button_evt_t) * event_pool_size);
    }

    return 0;
}

static void bubbleSort(int *arr, int n)
{
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            //如果前面的数比后面大，进行交换
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

static int button_longpress_check(button_t *b, int press_time)
{
    int ret = 0;

    for (int i = 0; i < b->press_time_cnt; i++) {
        if (press_time == b->press_time[i]) {
            ret = -1;
            break;
        }
    }

    return ret;
}

static int event_map_button(event_node_t *e_node)
{
    int button_count = e_node->button_count;
    button_evt_t *buttons = e_node->buttons;
    int ret = 0;

    for (int i = 0; i < button_count; i++) {
        button_t *b = button_find(buttons[i].button_id);

        ret = 0;
        if (buttons[i].event_id == BUTTON_PRESS_LONG_DOWN) {
            ret = button_longpress_check(b, buttons[i].press_time);
        }

        if (ret < 0) {
            continue;
        }

        if (buttons[i].event_id == BUTTON_PRESS_DOUBLE || buttons[i].event_id == BUTTON_PRESS_TRIPLE) {
            b->event_flag |= (1 << BUTTON_PRESS_DOWN);
            b->event_flag |= (1 << BUTTON_PRESS_UP);
        } else {
            b->event_flag |= (1 << buttons[i].event_id);
        }

        if (buttons[i].event_id == BUTTON_PRESS_LONG_DOWN) {
            b->press_time_cnt ++;
            b->press_time = aos_realloc_check(b->press_time, sizeof(int) * b->press_time_cnt);
            b->press_time[b->press_time_cnt - 1] = buttons[i].press_time;
            bubbleSort(b->press_time, b->press_time_cnt);
        }
    }

    return ret;
}

int button_add_gpio(int button_id, int gpio_pin, button_gpio_level_t active_level)
{
    button_t *b;
    aos_check_return_einval((gpio_pin >= 0) && ((active_level == LOW_LEVEL) || (active_level == HIGH_LEVEL)));

    int ret = buttton_check(button_id);

    if (ret < 0) {
        return -EINVAL;
    }

    ret = button_new(&b, BUTTON_TYPE_GPIO);

    if (!ret) {
        b->button_id = button_id;

        gpio_button_param_t *param = (gpio_button_param_t *)b->param;

        param->pin_id       = gpio_pin;
        param->active_level = active_level;

        button_add(b);
        button_set_ops(b, &gpio_ops);
        BUTTON_OPS(b, init);
    }

    return ret;
}

int button_add_adc(int button_id, char *adc_name, int adc_channel, int vol_ref, int vol_range)
{
    button_t *b;
    aos_check_return_einval(adc_name && vol_ref);

    int ret = buttton_check(button_id);

    if (ret < 0) {
        return -EINVAL;
    }

    ret = button_new(&b, BUTTON_TYPE_ADC);

    if (!ret) {
        b->button_id = button_id;

        adc_button_param_t *param = (adc_button_param_t *)b->param;

        strncpy(param->adc_name, adc_name, ADC_NAME_MAX - 1);
        param->adc_name[ADC_NAME_MAX - 1] = '\0';
        param->channel  = adc_channel;
        param->vref     = vol_ref;
        param->range    = vol_range;

        button_add(b);
        button_set_ops(b, &adc_ops);
        BUTTON_OPS(b, init);
        g_button_srv.adc_flag = 1;
        aos_timer_start(&g_button_srv.tmr);
    }

    return ret;
}

int button_add_event(int evt_id, button_evt_t *buttons, int button_count, button_evt_cb_t evt_cb, void *priv)
{
    event_node_t *e_node;

    aos_check_return_einval(buttons && button_count && evt_cb && button_count < 4);
    int ret = event_check(evt_id);

    if (ret < 0) {
        return -EINVAL;
    }

    ret = event_param_check(buttons, button_count);
    
    if (ret < 0) {
        LOGE(TAG, "button event(%d) add failed!", evt_id);
        return -1;
    }

    int cnt = event_buttons_count(buttons, button_count);
    
    ret = event_new(&e_node, cnt);

    if (!ret) {
        e_node->event_id     = evt_id;
        e_node->evt_cb       = evt_cb;
        e_node->priv         = priv;
        e_node->button_count = cnt;
        
        event_copy(e_node, buttons, button_count);
        event_add(e_node);
        event_pool_new(cnt);
        event_map_button(e_node);
    }

    return ret;
}

int button_is_pressed(int button_id, bool *pressed)
{
    button_t *b = button_find(button_id);
    if(b == NULL) {
        LOGE(TAG, "button not found!");
        return -1;
    }

    *pressed = b->is_pressed;

    return 0;
}

int button_init(void)
{
    if (g_button_srv.inited) {
        LOGE(TAG, "button had inited!");
        return -1;
    }

    memset(&g_button_srv, 0x00, sizeof(g_button_srv));
    aos_timer_new_ext(&g_button_srv.tmr, button_timer_entry, NULL, BUTTON_SCAN_TIME, 0, 0);
    slist_init(&g_button_srv.button_head);
    slist_init(&g_button_srv.event_node_head);
    g_button_srv.inited = 1;

    return 0;
}

int button_deinit(void)
{
    if (!g_button_srv.inited) {
        LOGE(TAG, "button not inited!");
        return -1;
    }

    aos_timer_stop(&g_button_srv.tmr);
    aos_timer_free(&g_button_srv.tmr);

    button_t *b = NULL;
    slist_t *temp;
    slist_for_each_entry_safe(&g_button_srv.button_head, temp, b, button_t, next) {
        BUTTON_OPS(b, deinit);

        button_remove(b);
        button_destroy(b);
    }

    event_node_t *e_node = NULL;
    slist_for_each_entry_safe(&g_button_srv.event_node_head, temp, e_node, event_node_t, next) {
        event_remove(e_node);
        event_destory(e_node);
    }

    aos_free(g_button_srv.event_pool);
    g_button_srv.event_pool_size = 0;
    g_button_srv.inited = 0;
    return 0;
}
