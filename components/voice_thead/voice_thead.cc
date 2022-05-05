/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/aos.h>
#include <aos/kernel.h>

#include <yoc/mic.h>
#include <yoc/mic_port.h>
#include "voice_thead.h"

#include <cxvision/cxvision.h>
#include "ulog/ulog.h"

#include "inference.h"
#include "post_process.h"


static const char *TAG = "voice_thead";

#define MIN(x, y) ((x) > (y) ? (y) : (x))

#define FRAME_SIZE ((16000 / 1000) * (16 / 8) * 20)  /* 640 */

static mic_event_t mic_evt;
static int g_kws_en = 1;

using MyCmdMessageT = posto::Message<thead::voice::proto::SessionMsg>;

static std::shared_ptr<posto::Participant> participant_;
static std::shared_ptr<posto::Reader<MyCmdMessageT>> reader_;

static void pcm_control_update(voice_t *v, int enable) {
  LOGD(TAG, "pcm_control_update enable %d", enable);
  if (enable) {
    aos_sem_signal(&v->sem);
  }
}

static void voice_event(void *priv, voice_evt_id_t evt_id, void *data, int len) {
  mic_event_id_t mic_evt_id;

  if (evt_id == VOICE_KWS_EVT) {
    if (g_kws_en == 0) {
      return;
    }
    mic_evt_id = MIC_EVENT_SESSION_START;
  } else if (evt_id == VOICE_DATA_EVT) {
    mic_evt_id = MIC_EVENT_PCM_DATA;
  } else if (evt_id == VOICE_SILENCE_EVT) {
    mic_evt_id = MIC_EVENT_SESSION_STOP;
  } else {
    LOGE(TAG, "unkown event id");
    return;
  }

  mic_evt(priv, mic_evt_id, data, len);
}

static void voice_session_sub(void) {
  participant_ = posto::Domain::CreateParticipant("cmd_consumer");
  reader_ = participant_->CreateReader<MyCmdMessageT>("session_cmd",
      [] (const std::shared_ptr<MyCmdMessageT>& msg) {
        mic_kws_t result = {0};
        int data_size = 0;
        voice_evt_id_t evt_id = VOICE_INVALID_EVT;

        voice_t *v = (voice_t *)mic_get_privdata();

        switch (msg->body().cmd_id()) {
            case thead::voice::proto::BEGIN:
              evt_id = VOICE_KWS_EVT;
              v->state = VOICE_STATE_BUSY;

              memcpy(result.word, msg->body().kws_word().c_str(), 32);
              result.id = msg->body().kws_id();
              result.score = msg->body().kws_score();
              data_size = sizeof(mic_kws_t);
            break;

            case thead::voice::proto::END:
            case thead::voice::proto::TIMEOUT:
              evt_id = VOICE_SILENCE_EVT;
              v->state = VOICE_STATE_IDLE;
            break;

            default:
            break;
        }

        voice_event(v->priv, evt_id, (void *)&result, data_size);
    });
}

static int voice_fake_wakeup(voice_t *v, int mode)
{
  voice_set_wakeup(mode);

  return 0;
}

static void plugin_task_entry(void *arg) {
  int data_size = 0;
  voice_t *v = (voice_t *)arg;

  static const std::string json_str = R"({
    "pipeline_0": {
      "data_input": {
        "plugin": "DataInput",
        "props": {
            "chn_num": "3",
            "interleaved": "0",
            "pcm_bits": "16",
            "sample_rate": "16000",
            "frame_ms": "10"
        },
        "next": ["pre_process", "record_process#0"]
      },
      "pre_process": {
        "plugin": "PreProcess",
        "next": ["inference", "post_process#0", "vad_process#0", "record_process#1"],
        "thread": {
          "priority": "28",
          "stack_size": "32768"
        }
      },
      "inference": {
        "device_id": "0",
        "plugin": "Inference",
        "props": {
          "model_path": "models/xxxx/yyyy"
        },
        "next": ["post_process#1", "vad_process#1"],
        "thread": {
          "priority": "28",
          "stack_size": "32768"
        }
      },
      "vad_process": {
        "plugin": "VadProc",
        "next": "post_process#2"
      },
      "record_process": {
        "plugin": "RecordProc",
        "props": {
            "rec_group": "2"
        }
      },
      "post_process": {
        "plugin": "PostProcess"
      }
    }
  })";

  cx::GraphManager graphMgr(json_str);

  if (!graphMgr.Start()) {
    LOGD(TAG, "Start graphs failed.\n");
  }

  voice_session_sub();

  char *pcm_data = (char *)aos_malloc_check(FRAME_SIZE);

  while (v->task_running) {
    aos_sem_wait(&v->sem, AOS_WAIT_FOREVER);

    //LOGI(TAG, "~~~~~~task_running~~~~~~\n");
    while ((data_size = voice_get_pcm_data(pcm_data, FRAME_SIZE)) > 0 || v->state == VOICE_STATE_BUSY) {

      if (data_size > 0) {
        voice_event(v->priv, VOICE_DATA_EVT, pcm_data, data_size);
      }
      aos_msleep(20);
    }
  }

  aos_task_exit(0);
}


static int mic_adaptor_init(mic_t *mic, mic_event_t event)
{
  voice_t *v = (voice_t *)aos_malloc_check(sizeof(voice_t));

  mic_evt = event;

  v->priv = mic;

  int ret = aos_sem_new(&v->sem, 0);
  if (ret < 0) {
    aos_free(v);
    return -1;
  }

  aos_task_new_ext(&v->plugin_task, "voice_thead", &plugin_task_entry, v, 1024 * 8, AOS_DEFAULT_APP_PRI);

  mic_set_privdata(v);

  return 0;
}

static int mic_adaptor_deinit(mic_t *mic)
{
  voice_t * v = (voice_t *)mic_get_privdata();

  aos_sem_free(&v->sem);

  aos_free(v);

  return 0;
}


static int mic_adaptor_start(mic_t *mic)
{
  voice_t * v = (voice_t *)mic_get_privdata();

  v->task_running = 1;

  return 0;
}


static int mic_adaptor_stop(mic_t *mic)
{
  voice_t * v = (voice_t *)mic_get_privdata();

  v->task_running = 0;

  return 0;
}

static int mic_adaptor_pcm_data_control(mic_t *mic, int enable)
{
  voice_t *v = (voice_t *)mic_get_privdata();

  pcm_control_update(v, enable);

  return 0;
}

/* update the wakeup voice sate to ai side to allow voice cut */
static int mic_adaptor_fake_wakeup(mic_t *mic, int en, int flag)
{
  voice_t * v = (voice_t *)mic_get_privdata();

  voice_fake_wakeup(v, en);

  return 0;
}


static mic_ops_t voice_ops = {
  .init    = mic_adaptor_init,
  .deinit  = mic_adaptor_deinit,

  .start   = mic_adaptor_start,
  .stop    = mic_adaptor_stop,
  .pcm_data_control = mic_adaptor_pcm_data_control,

  // .set_param = mic_adaptor_set_param,
  // .audio_control = NULL,

  .kws_wakeup = mic_adaptor_fake_wakeup,
  // .debug_control    = mic_adaptor_debug_control,
};

void mic_voice_register(void)
{
  mic_ops_register(&voice_ops);
}