/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <memory>

#include <cxvision/cxvision.h>
#include "ulog/ulog.h"
#include <aos/ringbuffer.h>
#include <aos/aos.h>

#define TAG "post"

#define FRAME_SIZE ((16000 / 1000) * (16 / 8) * 10) /* 320 */
#define RINGBUFFER_LEN  (FRAME_SIZE * 200 + 1)  /* 2s */


static dev_ringbuf_t dev_rb;

extern "C" int voice_get_pcm_data(void *data, int len) {
  uint8_t temp[640] = {0};
  int read_len = len;

  if (ringbuffer_empty(&dev_rb)) {
    return 0;
  }

  if (ringbuffer_available_read_space(&dev_rb) < len) {
    read_len = ringbuffer_available_read_space(&dev_rb);
  }

  // LOGD(TAG, "[rb read] read_len: %d\n", read_len);


  ringbuffer_read(&dev_rb, (uint8_t *)temp, read_len);

  memcpy(data, temp, read_len);

  return read_len;
}

using SspOutMessageT   = posto::Message<thead::voice::proto::SspOutMsg>;
using InferOutMessageT = posto::Message<thead::voice::proto::InferOutMsg>;
using VadOutMessageT   = posto::Message<thead::voice::proto::VadOutMsg>;
using MyCmdMessageT    = posto::Message<thead::voice::proto::SessionMsg>;

namespace cpt {

class PostProcess : public cx::PluginBase {
public:
  PostProcess();
private:
  char *rbuf;
  int kws_chn;

  std::shared_ptr<posto::Participant> participant_;
  std::shared_ptr<posto::Writer<MyCmdMessageT>> writer_;

public:
  bool Init(const std::map<std::string, std::string>& props) override;
  bool DeInit() override;
  bool Process(const std::vector<cx::BufferPtr>& data_vec) override;
};

PostProcess::PostProcess()
{
  rbuf = NULL;
  kws_chn = 0;
}

bool PostProcess::Init(const std::map<std::string, std::string>& props) {
  const auto& iter = props.find("max_time_ms");
  if (iter != props.end()) {
    LOGD(TAG, "max_time_ms: %s\n", iter->second.c_str());
    // max_time_ms = atoi(iter->second.c_str());
  }

  participant_ = posto::Domain::CreateParticipant("cmd_producer");
  writer_ = participant_->CreateWriter<MyCmdMessageT>("session_cmd");

  rbuf = (char *)malloc(RINGBUFFER_LEN);
  ringbuffer_create(&dev_rb, rbuf, RINGBUFFER_LEN);

  // aos_timer_new_ext(&session_timer, timer_entry, this, max_time_ms, 0, 0);

  // set_kws_state(0);
  // set_vad_state(0);

  kws_chn = 0;

  return true;
}

bool PostProcess::DeInit() {
  // aos_timer_free(&session_timer);

  ringbuffer_destroy(&dev_rb);
  free(rbuf);

  return true;
}

bool PostProcess::Process(const std::vector<cx::BufferPtr>& data_vec) {
  auto ptr0 = data_vec.at(0); // ssp data
  auto ptr1 = data_vec.at(1); // kws info
  auto ptr2 = data_vec.at(2); // vad info

  int pub_flag = 0;

  // int next_kws_state = 0;
  auto msg = std::make_shared<MyCmdMessageT>();

  // ssp data
  if (ptr0) {
    auto iMemory0 = ptr0->GetMemory(0);
    auto iMeta0 = ptr0->GetMetadata<SspOutMessageT>("ssp_param");

    int16_t *data = (int16_t *)iMemory0->data();

    // LOGE(TAG, "PostProcess ringbuffer ssp data frame %d, chn_num %d, kws_chn %d", iMeta0->frame, iMeta0->chn_num, kws_chn);
    // if (get_kws_state() == 0) {
    //   return true;
    // }

    if (ringbuffer_full(&dev_rb)) {
      LOGE(TAG, "ringbuffer is full");
    }

    ringbuffer_write(&dev_rb, (uint8_t *)&data[kws_chn * iMeta0->body().frame()], iMeta0->body().frame() * sizeof(int16_t));

    return true;
  }

  // kws state
  if (ptr1) {
    auto iMeta1 = ptr1->GetMetadata<InferOutMessageT>("kws_param");
    kws_chn = iMeta1->body().kws_chn();

    if (iMeta1->body().first_wakeup() == false) {
      return true;
    }

    // next_kws_state = 1;

    LOGD(TAG, "  Port[1].Meta[\"kws_param\"]: kws_chn %d, kws_id %d, kws_score %d\n", 
          iMeta1->body().kws_chn(), iMeta1->body().kws_id(), iMeta1->body().kws_score());

    ringbuffer_clear(&dev_rb);

    pub_flag = 1;
    msg->body().set_cmd_id(thead::voice::proto::BEGIN);
    msg->body().set_kws_id(iMeta1->body().kws_id());
    msg->body().set_kws_score(iMeta1->body().kws_score());
    msg->body().set_kws_word("nihaoxinbao");
  }

  // vad state
  if (ptr2) {
    auto iMeta2 = ptr2->GetMetadata<VadOutMessageT>("vad_param");
    // set_vad_state(0);

    pub_flag = 1;
    msg->body().set_cmd_id(thead::voice::proto::END);
    LOGD(TAG, "  Port[2].Meta[\"vad_param\"]: vad %d\n", iMeta2->body().vad_status());
  }

  // in kws period
  // if (get_kws_state() == 1) {
  //   // continue kws
  //   if (next_kws_state == 1) {
  //     set_vad_state(1);

  //     LOGD(TAG, "  continue kws.\n");

  //     // restart timer
  //     // if (aos_timer_is_valid(&session_timer)) {
  //     //   aos_timer_stop(&session_timer);
  //     //   aos_timer_start(&session_timer);
  //     // }
  //   } else {
  //     // vad to end kws
  //     if (get_vad_state() == 0) {
  //       set_kws_state(0);

  //       // if (aos_timer_is_valid(&session_timer)) {
  //       //   aos_timer_stop(&session_timer);
  //       // }

  //       pub_flag = 1;
  //       msg->body().set_cmd_id(thead::voice::proto::END);
  //     }
  //   }
  // } else {
  //   set_kws_state(next_kws_state);

  //   if (next_kws_state) {
  //     set_vad_state(1);

  //     // if (aos_timer_is_valid(&session_timer)) {
  //     //   aos_timer_start(&session_timer);
  //     // }
  //   }
  // }

  // pub msg to sub
  if (pub_flag) {
    LOGD(TAG, "pub msg: cmd_id %d\n", msg->body().cmd_id());
    writer_->Write(msg);
  }

  // LOGD(TAG, "result: kws_state %d, vad_state %d, next_kws_state %d\n", cur_kws_state, vad_state, next_kws_state);

  return true;
}

CX_REGISTER_PLUGIN(PostProcess);

}  // namespace cpt
