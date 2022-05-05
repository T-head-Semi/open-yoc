/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <memory>

#include <cxvision/cxvision.h>
#include "ulog/ulog.h"

#define TAG "Vad"

#define VAD_FRAME_COUNT 60   // 10ms per frame
#define VAD_BREAK_PER   30   /*激活率小于改百分比进行断句*/
#define VAD_IGN_COUNT   60   /*唤醒后先忽略一定量数据*/
#define VAD_MAX_COUNT   1000  /*唤醒后一次交互的最长时间*/

using SspOutMessageT   = posto::Message<thead::voice::proto::SspOutMsg>;
using InferOutMessageT = posto::Message<thead::voice::proto::InferOutMsg>;
using VadOutMessageT   = posto::Message<thead::voice::proto::VadOutMsg>;

namespace cpt {

class VadProc : public cx::PluginBase {
public:
  VadProc();
  bool Init(const std::map<std::string, std::string>& props) override;
  bool DeInit() override;
  bool Process(const std::vector<cx::BufferPtr>& data_vec) override;

private:
  int kws_chn;
  int kws_state;
  int16_t *ssp_data;

  int vad_active_cnt; /*激活点数*/
  int vad_frame_cnt;  /*统计点数*/
  int vad_max_cnt;    /*一次交互最长点数*/
};

VadProc::VadProc()
{
  kws_chn   = 0;
  kws_state = 0;

  vad_active_cnt = 0;
  vad_frame_cnt = 0;
  vad_max_cnt   = 0;

  ssp_data = NULL;
}

bool VadProc::Init(const std::map<std::string, std::string>& props) {
  kws_chn   = 0;
  kws_state = 0;

  vad_active_cnt = 0;
  vad_frame_cnt = 0;
  vad_max_cnt   = 0;

  return true;
}

bool VadProc::DeInit() {
  return true;
}

bool VadProc::Process(const std::vector<cx::BufferPtr>& data_vec) {
  auto ptr0 = data_vec.at(0);
  auto ptr1 = data_vec.at(1);

  int vad_break = 0;

  int frame_vad = 0x0F;

  // sspout data
  if (ptr0) {
    auto iMeta0 = ptr0->GetMetadata<SspOutMessageT>("ssp_param");

    frame_vad = iMeta0->body().vad_res();

    // auto iMemory0 = ptr0->GetMemory(0);
    // ssp_data = (int16_t *)iMemory0->data();

    // LOGD(TAG, "  Port[0].Meta[\"sspout_param\"]: chan %d, len %d, vad %d\n", iMeta0->chn_num, iMeta0->frame, iMeta0->vad_res);
  }

  // kws state
  if (ptr1) {
    auto iMeta1 = ptr1->GetMetadata<InferOutMessageT>("kws_param");

    kws_chn = iMeta1->body().kws_chn();

    if (iMeta1->body().first_wakeup() == false) {
      return true;
    }

    kws_state = 1;
    vad_active_cnt = 0;
    vad_frame_cnt = 0;
    vad_max_cnt   = 0;

    //LOGD(TAG, "  Port[1].Meta[\"kws_param\"]: kws_chn %d, kws_id %d.\n", iMeta1->kws_chn, iMeta1->kws_id);
  }

  if (kws_state == 0) {
    /*没唤醒过，不处理*/
    return true;
  }

  /*VAD点总数统计*/
  vad_frame_cnt ++;
  vad_max_cnt++;
  // LOGD(TAG, "%d %d", vad_frame_cnt, vad_max_cnt);

  /*当前帧vad状态，并统计激活数*/
  if (((frame_vad >> kws_chn) & 0x1) != 0) {
    vad_active_cnt ++;
  }

  /*达到最大交互时间*/
  if (vad_max_cnt >= VAD_MAX_COUNT) {
    vad_break = 1;
    kws_state = 0;
    LOGD(TAG, "VAD reach max cnt.");
    goto VAD_BREAK;
  }

  /*忽略刚唤醒的数据，控制最小断句时间*/
  if (kws_state == 1) {
    if (vad_frame_cnt >= VAD_IGN_COUNT) {
      int vad_active_per = (vad_active_cnt * 100 / vad_frame_cnt);
      LOGD(TAG, "VAD start active %d", vad_active_per);

      if (vad_active_per > VAD_BREAK_PER) {
        kws_state = 2; /*有声音起点进入可断句状态*/
        LOGD(TAG, "Fin vad start");
      }

      /*一帧结束重新统计*/
      vad_active_cnt = vad_frame_cnt  = 0;
    }
    return true;
  }

  /*非断句状态返回*/
  if (kws_state != 2) {
    return true;
  }

  /*进入断句状态*/
  if (vad_frame_cnt >= VAD_FRAME_COUNT ) {
    LOGD(TAG, "VAD active %d", (vad_active_cnt * 100 / vad_frame_cnt));

    if ( (vad_active_cnt * 100 / vad_frame_cnt) < VAD_BREAK_PER ) {
      /*断句*/
      vad_break = 1;
      kws_state = 0;
      LOGD(TAG, "VAD break");
    }
    /*一帧结束重新统计*/
    vad_active_cnt = vad_frame_cnt  = 0;
  }

VAD_BREAK:
  /*发送断句事件*/
  if (vad_break) {
    auto oMeta = std::make_shared<VadOutMessageT>();
    oMeta->body().set_vad_status(0);

    auto output = std::make_shared<cx::Buffer>();
    output->SetMetadata("vad_param", oMeta);

    Send(0, output);
  }

  return true;
}

CX_REGISTER_PLUGIN(VadProc);

}  // namespace cpt
