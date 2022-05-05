/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include "aui_nlp.h"
#include "aui_cloud/ex_cjson.h"

#define TAG "nlpiot"

#define DEFAULT_VOLUME_STEP 20

#if defined(APP_PWM_EN) && APP_PWM_EN
static void light_value_copy(char *dstStr, const char *srcStr)
{
    while ('\0' != *srcStr) {
        if (('%' != *srcStr) && ('\"' != *srcStr)) {
            *dstStr++ = *srcStr++;
        } else {
            srcStr++;
        }
    }
    *dstStr = '\0';

    return;
}

/**
 * 根据NLP信息设置亮度
*/
int aui_nlp_action_set_light(cJSON *js)
{
    int     ret        = -1;
    int32_t value      = -1;
    cJSON * norm_value = NULL;
    cJSON * tag        = NULL;
    cJSON * light_type = NULL;

    if ((tag = cJSON_GetObjectItemByPath(js, "payload.semantics[0].slots.light[0].tag")) &&
        cJSON_IsString(tag) && (strcmp(tag->valuestring, "ALI.NUMBER") == 0)) {
        /* 检测到固定值 */
        norm_value =
            cJSON_GetObjectItemByPath(js, "payload.semantics[0].slots.light[0].norm_value");
    } else if ((tag = cJSON_GetObjectItemByPath(js, "payload.semantics[0].slots.ratio[0].tag")) &&
               cJSON_IsString(tag) && (strcmp(tag->valuestring, "USER.HOUSE_PERCENTAGE") == 0)) {
        /* 检测到固定百分比 */
        norm_value =
            cJSON_GetObjectItemByPath(js, "payload.semantics[0].slots.ratio[0].norm_value");
    }

    if (cJSON_IsString(norm_value)) {
        /*  convert string to number, remove other characters */
        char numStr[strlen(norm_value->valuestring) + 1];
        light_value_copy(numStr, norm_value->valuestring);
        value = atoi(numStr);
    } else if ((light_type = cJSON_GetObjectItemByPath(
                    js, "payload.semantics[0].slots.lightType[0].norm_value")) &&
               cJSON_IsString(light_type)) {
        /* 调至最亮/调至最暗 */
        if (json_string_eq(light_type, "lightType_min")) {
            value = PWM_LED_LIGHT_MIN;
        } else if (json_string_eq(light_type, "lightType_max")) {
            value = PWM_LED_LIGHT_MAX;
        }
    }

    cJSON *action_params_name = cJSON_GetObjectItemByPath(js, "payload.action_params[0].name");
    if (json_string_eq(action_params_name, "degree")) {
        cJSON *action_params_value =
            cJSON_GetObjectItemByPath(js, "payload.action_params[0].value");
        if (-1 == value) {
            value = DEFAULT_VOLUME_STEP;
        }
        int32_t brightness = app_pwm_led_get_brightness();
        if (json_string_eq(action_params_value, "up")) {
            ret = app_pwm_led_set_brightness(brightness + value);
        } else if (json_string_eq(action_params_value, "down")) {
            ret = app_pwm_led_set_brightness(brightness - value);
        }
    } else if (value >= 0) {
        /* 设置亮度到某个固定值 */
        ret = app_pwm_led_set_brightness(value);
    }
    return ret;
}
#endif