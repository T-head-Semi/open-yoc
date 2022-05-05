/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <smart_audio.h>

#include "aui_cloud/ex_cjson.h"
#include "aui_nlp.h"

#define TAG "nlpmit"

/*
一次交互会有asr和nlp两次事件，但两次事件的task id相同，
如果asr处理是对该值赋值，说明要忽略后续的nlp处理。
*/
static char g_nlp_task_id[40] = {0};

/**
 * 解析ASR&NLP信息
*/
int aui_nlp_proc_mit(cJSON *js, const char *json_text)
{
    int ret = 0;
    uint8_t need_resume = 1;

    cJSON *name = cJSON_GetObjectItemByPath(js, "header.name");

    if (!cJSON_IsString(name)) {
        return AUI_CMD_PROC_ERROR;
    }

    //LOGI(TAG, "%s", json_text);
    if (strcmp(name->valuestring, "RecognitionCompleted") == 0) {
        /* ASR 结果 */
        cJSON *asr_result = cJSON_GetObjectItemByPath(js, "payload.result");
        if (cJSON_IsString(asr_result)) {
            LOGD(TAG, "{asr_text:\"%s\"}", asr_result->valuestring);
            ret = aui_nlp_proc_textcmd(asr_result->valuestring);
            if (ret == 0) {
                cJSON *task_id = cJSON_GetObjectItemByPath(js, "header.task_id");
                if (cJSON_IsString(task_id)) {
                    /* 忽略后面的NLP处理 */
                    strncpy(g_nlp_task_id, task_id->valuestring, sizeof(g_nlp_task_id) - 1);
                    g_nlp_task_id[sizeof(g_nlp_task_id) - 1] = 0;
                }
            }
            return 0;
        }

        return AUI_CMD_PROC_MATCH_NOACTION;
    } else if (strcmp(name->valuestring, "DialogResultGenerated") == 0) {
        int done = 0;

        cJSON *task_id = cJSON_GetObjectItemByPath(js, "header.task_id");
        if (json_string_eq(task_id, g_nlp_task_id)) {
            g_nlp_task_id[0] = '\0';
            return 0;
        }

        char *music_url = aui_nlp_action_get_music_url(js);
        if (music_url) {
            smtaudio_stop(MEDIA_MUSIC);
            done = 1;
        }

        /* NLP 结果，TTS前执行动作 */
        cJSON *action = cJSON_GetObjectItemByPath(js, "payload.action");
        if (cJSON_IsString(action)) {
            LOGD(TAG, "{nlp_action:\"%s\"}", action->valuestring);
            if (strcmp(action->valuestring, "Action://audio/set_volumn") == 0) {
                ret = aui_nlp_action_set_volume(js, &need_resume);
                if(need_resume == 0) {
                    smtaudio_enable_ready_list(0);
                }
                done = 1;
            } else if (strcmp(action->valuestring, "Action://audio/pause") == 0) {
                smtaudio_enable_ready_list(0);
                done = 1;
            } else if (strcmp(action->valuestring, "Action://audio/resume") == 0) {
                smtaudio_enable_ready_list(1);
                smtaudio_resume();
                done = 1;
            } else if (strcmp(action->valuestring, "Action://audio/next") == 0) {
                LOGI(TAG, "Do %s\n", action->valuestring);
                done = 1;
            } else if (strcmp(action->valuestring, "Action://audio/previous") == 0) {
                LOGI(TAG, "Do %s\n", action->valuestring);
                done = 1;
            } else if (strcmp(action->valuestring, "Action://alarm/add_alarm") == 0
                       || strcmp(action->valuestring, "Action://alarm/set_alarm") == 0) {
                int result = aui_nlp_action_set_alarm(js, NLP_ACTION_ALARM_ADD);
                if (result == -2) {
                    cJSON *spoken_text = cJSON_GetObjectItemByPath(js, "payload.spoken_text");
                    if (cJSON_IsString(spoken_text)) {
                        cJSON_SetValuestring(spoken_text, "已设置过此闹钟");
                    }
                } else if (result == -1 || result == -3) {
                    cJSON *spoken_text = cJSON_GetObjectItemByPath(js, "payload.spoken_text");
                    if (cJSON_IsString(spoken_text)) {
                        cJSON_SetValuestring(spoken_text, "设置闹钟个数达到上限");
                    }                    
                }
                done = 1;
            } else if (strcmp(action->valuestring, "Action://alarm/delete_alarm") == 0) {
                aui_nlp_action_set_alarm(js, NLP_ACTION_ALARM_DEL);
                done = 1;
#if defined(APP_PWM_EN) && APP_PWM_EN
            } else if (strcmp(action->valuestring, "Action://house_control/start_device") == 0) {
                app_pwm_led_control(1);
                done = 1;
            } else if (strcmp(action->valuestring, "Action://house_control/stop_device") == 0) {
                app_pwm_led_control(0);
                done = 1;
            } else if (strcmp(action->valuestring, "Action://house_control/set_light") == 0) {
                ret = aui_nlp_action_set_light(js);
                if (ret == 0) {
                    done = 1;
                }
#endif //APP_PWM_EN
            }
        }
        /* 如果有音乐，播放音乐 */
        if (music_url) {
            smtaudio_start(MEDIA_MUSIC, music_url, 0, 0);
        } else if(smtaudio_get_state() != SMTAUDIO_STATE_MUTE) {
            /* NLP 结果，语音交互 */
            cJSON *spoken_text = cJSON_GetObjectItemByPath(js, "payload.spoken_text");
            if (cJSON_IsString(spoken_text)) {
                LOGD(TAG, "{nlp_tts: \"%s\"}", spoken_text->valuestring);
                if (strlen(spoken_text->valuestring) > 0) {
                    app_aui_cloud_tts_run(spoken_text->valuestring, 0);
                    // app_aui_cloud_tts_wait_start();
                }
                done = 1;
            } else if(done) {
                /* 如果云端不带TTS，本地处理完毕，播放提示音‘好的’ */
                LOGD(TAG, "好的！");
                app_aui_cloud_tts_run("好的", 0);
                // app_aui_cloud_tts_wait_start();
            }
        }

        return ret < 0 ? ret : (done ? 0 : AUI_CMD_PROC_MATCH_NOACTION);
    } else {
        return AUI_CMD_PROC_NOMATCH;
    }

    return 0;
}
