/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef _AUI_NLP_H_
#define _AUI_NLP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yoc/aui_cloud.h>
#include <cJSON.h>
#include <media.h>
#include "aui_cloud/app_aui_cloud.h"

#define NLP_ACTION_ALARM_ADD    1
#define NLP_ACTION_ALARM_DEL    2

int json_string_eq(cJSON *js, const char *str);

/* ASR文本传入进行命令处理 */
int aui_nlp_proc_textcmd(const char *json_text);

/* nlp action */
int aui_nlp_action_set_volume(cJSON *js, uint8_t *need_resume);
int aui_nlp_action_play_pause(void);
int aui_nlp_action_play_resume(void);
int aui_nlp_action_set_light(cJSON *js);
char * aui_nlp_action_get_music_url(cJSON *js);
int aui_nlp_action_set_alarm(cJSON *js, int action);

int aui_nlp_proc_mit(cJSON *js, const char *json_text);
#endif
