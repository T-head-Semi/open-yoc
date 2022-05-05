/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <memory>

#include <aos/kv.h>
#include <cxvision/cxvision.h>
#include <ulog/ulog.h>
#include <aos/ringbuffer.h>
#include <aos/aos.h>

#define TAG "Record"

#define SSP_RECORD_NUM        3

#define FA_READ_CNT         450  // 10ms per time, send 4.5s data
#define FA_RINGBUF_LEN      (16 * 2 * 3 * 3000)  /* store fault wakeup data: 3sec, 3chn */

using DataInputMessageT = posto::Message<thead::voice::proto::DataInputMsg>;
using SspOutMessageT    = posto::Message<thead::voice::proto::SspOutMsg>;
using SessionMessageT   = posto::Message<thead::voice::proto::SessionMsg>;

namespace cpt {

class RecordProc : public cx::PluginBase {

public:
  RecordProc();
  bool Init(const std::map<std::string, std::string>& props) override;
  bool DeInit() override;
  bool Process(const std::vector<cx::BufferPtr>& data_vec) override;

private:
  std::shared_ptr<posto::Participant> participant_;
  std::shared_ptr<posto::Reader<SessionMessageT>> session_reader_;

  int rec_group_;
  int wakeup_ringbuf_rec_;

  dev_ringbuf_t pcm_rb_;
  int pcm_read_cnt_;

  dev_ringbuf_t ssp_rb_;
  int ssp_read_cnt_;

private:
    int push_to_history_buffer(dev_ringbuf_t *ringbuffer, uint8_t *data, uint32_t length);
};

RecordProc::RecordProc()
{
  rec_group_ = wakeup_ringbuf_rec_ = pcm_read_cnt_ = 0;
}

int RecordProc::push_to_history_buffer(dev_ringbuf_t *ringbuffer, uint8_t *data, uint32_t length)
{
    uint32_t tmplen = 0;

    tmplen = ringbuffer->length - ringbuffer->widx;
    if(length <= tmplen) {
        memcpy((void*)&ringbuffer->buffer[ringbuffer->widx], (void*)data, length);
    } else {
        memcpy((void*)&ringbuffer->buffer[ringbuffer->widx], (void*)data, tmplen);
        memcpy((void*)ringbuffer->buffer, (uint8_t*)data + tmplen, length - tmplen);
    }

    ringbuffer->widx = (ringbuffer->widx + length) % (ringbuffer->length + 1);

    return length;
}

bool RecordProc::Init(const std::map<std::string, std::string>& props) {
  for (auto iter = props.begin(); iter != props.end(); ++iter) {
      if (strcmp(iter->first.c_str(), "rec_group") == 0) {
          rec_group_ = atoi(iter->second.c_str());
      }
  }

  wakeup_ringbuf_rec_ = 0;

  pcm_read_cnt_ = FA_READ_CNT + 1;
  ssp_read_cnt_ = FA_READ_CNT + 1;

  // store previous 4s data to buffer
  aos_kv_getint("wakeup_rec", &wakeup_ringbuf_rec_);
  if (wakeup_ringbuf_rec_) {
      char *pcm_ringbuff = (char *)malloc(FA_RINGBUF_LEN);
      ringbuffer_create(&pcm_rb_, pcm_ringbuff, FA_RINGBUF_LEN);

      char *ssp_ringbuff = (char *)malloc(FA_RINGBUF_LEN);
      ringbuffer_create(&ssp_rb_, ssp_ringbuff, FA_RINGBUF_LEN);
  }

  participant_ = posto::Domain::CreateParticipant("cmd_consumer");
  session_reader_ = participant_->CreateReader<SessionMessageT>("session_cmd",
      [this] (const std::shared_ptr<SessionMessageT>& msg) {
        LOGD(TAG, "Message got, cmd_id: %d\n", msg->body().cmd_id());
        switch (msg->body().cmd_id()) {
            case thead::voice::proto::BEGIN:
            pcm_read_cnt_ = 0;
            ssp_read_cnt_ = 0;
            break;

            case thead::voice::proto::END:
            case thead::voice::proto::TIMEOUT:
            break;

            default:
            break;
        }
    });

  return true;
}

bool RecordProc::DeInit() {

  return true;
}

extern "C" { extern void rec_copy_data(int index, uint8_t *data, uint32_t size); }
bool RecordProc::Process(const std::vector<cx::BufferPtr>& data_vec) {
  auto ptr0 = data_vec.at(0); // pcm data

  // pcm data
  if (ptr0) {
    auto iMemory0 = ptr0->GetMemory(0);
    auto iMeta0 = ptr0->GetMetadata<DataInputMessageT>("alsa_param");

    int len = iMeta0->body().frame();
    int chn = iMeta0->body().chn_num();
    int pcm_out_len = len * chn * iMeta0->body().format() / 8;
    int16_t *pcm_output = (int16_t*)malloc(pcm_out_len);

    if (pcm_output == NULL) {
      return false;
    }

    int16_t *input  = (int16_t *)iMemory0->data();
    for(int i = 0; i < len; i++) {
      for (int j = 0; j < chn; j ++) {
        pcm_output[i * chn + j] = input[len * j + i];
      }
    }

    if (wakeup_ringbuf_rec_) {
      //store to ringbuffer
      push_to_history_buffer(&pcm_rb_, (uint8_t *)pcm_output, pcm_out_len);

      // if wakeup, send the ringbuffer data to server
      if (pcm_read_cnt_ == 0) {
        pcm_rb_.ridx = (pcm_rb_.widx + pcm_out_len) % (pcm_rb_.length + 1);
        LOGD(TAG, "read_first pcm_rb_.widx: %d ridx: %d\n", pcm_rb_.widx, pcm_rb_.ridx);
      }

      if (pcm_read_cnt_ <= FA_READ_CNT) {
        pcm_read_cnt_ ++;

        int read_len = ringbuffer_read(&pcm_rb_, (uint8_t *)pcm_output, pcm_out_len);
        if (read_len != 0) {
          rec_copy_data(0, (uint8_t *)pcm_output, pcm_out_len);
        }
      }
    } else {
      rec_copy_data(0, (uint8_t*)pcm_output, pcm_out_len);
    }

    free(pcm_output);
  }

  if (rec_group_ > 1) {
    auto ptr1 = data_vec.at(1); // ssp data
    // ssp data
    if (ptr1) {
      auto iMemory1 = ptr1->GetMemory(0);
      auto iMeta1 = ptr1->GetMetadata<SspOutMessageT>("ssp_param");

      int alg_rec_num = iMeta1->body().chn_num();

      int ssp_out_len = iMeta1->body().frame() * alg_rec_num * sizeof(int16_t);

      int16_t *ssp_output = (int16_t *)malloc(ssp_out_len);
      if (ssp_output == NULL) {
        return false;
      }

      int16_t *ssp_data = (int16_t *)iMemory1->data();

      for(int i = 0; i < iMeta1->body().frame(); i++) {
        for (int j = 0; j < alg_rec_num; j ++) {
          ssp_output[i * alg_rec_num + j] = ssp_data[iMeta1->body().frame() * j + i];
        }
      }

      if (wakeup_ringbuf_rec_) {
        //store to ringbuffer
        push_to_history_buffer(&ssp_rb_, (uint8_t *)ssp_output, ssp_out_len);

        // if wakeup, send the ringbuffer data to server
        if (ssp_read_cnt_ == 0) {
          ssp_rb_.ridx = (ssp_rb_.widx + ssp_out_len) % (ssp_rb_.length + 1);
          LOGD(TAG, "read_first ssp_rb_.widx: %d ridx: %d\n", ssp_rb_.widx, ssp_rb_.ridx);
        }

        if (ssp_read_cnt_ <= FA_READ_CNT) {
          ssp_read_cnt_ ++;

          int read_len = ringbuffer_read(&ssp_rb_, (uint8_t *)ssp_output, ssp_out_len);
          if (read_len != 0) {
            rec_copy_data(1, (uint8_t *)ssp_output, ssp_out_len);
          }
        }
      } else {
        rec_copy_data(1, (uint8_t*)ssp_output, ssp_out_len);
      }

      free(ssp_output);
    }
  }

  return true;
}

CX_REGISTER_PLUGIN(RecordProc);

}  // namespace cpt
