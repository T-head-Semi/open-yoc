/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <memory>

#include <ulog/ulog.h>
#include <cxvision/cxvision.h>

#include "thead_kws.h"

#define TAG "infer"

#define MIN_WAKE_INTERV 15 /*15*20ms=300ms*/

#define CHN_NUM    3

using SspOutMessageT   = posto::Message<thead::voice::proto::SspOutMsg>;
using InferOutMessageT = posto::Message<thead::voice::proto::InferOutMsg>;

static int g_fake_wakeup = 0;

extern "C" void voice_set_wakeup(int mode)
{
    g_fake_wakeup = mode;
}

namespace cpt
{

class Inference : public cx::PluginBase
{
public:
    bool Init(const std::map<std::string, std::string> &props) override;
    bool DeInit() override;
    bool Process(const std::vector<cx::BufferPtr> &data_vec) override;

private:
    int inited_flag_;
    int *output_data_[CHN_NUM];

    int kws_chn_id_;
    int ref_chn_id_;

    int16_t *frame_buf_;
    size_t frame_buf_len_;

    void *temp_buf_;
    size_t temp_buf_len_;

    int merged_flag_;
    int kws_proc_cnt_;

    int max_score_;
};

bool Inference::Init(const std::map<std::string, std::string> &props)
{
    const auto &iter = props.find("model_path");

    if (iter != props.end()) {
        LOGD(TAG, "model_path: %s\n", iter->second.c_str());
    }

    for (int i = 0; i < CHN_NUM; i++) {
        output_data_[i] = (int *)malloc(2 * sizeof(int));
    }

    frame_buf_ = (int16_t *)malloc(320 * CHN_NUM * 2);
    merged_flag_  = 0;

    inited_flag_ = 0;

    ref_chn_id_ = CHN_NUM - 1;

    kws_proc_cnt_ = 0;

    g_fake_wakeup = 0;

    max_score_   = 0;

    return true;
}

bool Inference::DeInit()
{
    if (temp_buf_) {
        free(temp_buf_);
    }

    return true;
}

bool Inference::Process(const std::vector<cx::BufferPtr> &data_vec)
{
    auto iMemory = data_vec.at(0)->GetMemory(0);
    int16_t *data_in = (int16_t *)iMemory->data();

    auto iMeta = data_vec.at(0)->GetMetadata<SspOutMessageT>("ssp_param");

    if (inited_flag_ == 0) {
        int ret  = thead_kws_init(iMeta->body().chn_num(), iMeta->body().sample_rate(), &frame_buf_len_, &temp_buf_len_);
        int temp_buf_byte_len = temp_buf_len_ * iMeta->body().chn_num() * sizeof(int16_t);
        temp_buf_ = malloc(temp_buf_byte_len);
        LOGD(TAG, "thead_kws_init. chn_num: %d, frame %d, temp:%p(%ld), ret %d\n", 
             iMeta->body().chn_num(), iMeta->body().frame(), temp_buf_, temp_buf_byte_len, ret);
        inited_flag_ = 1;
    }

    // 2 10ms frames merge to 20ms frame
    merged_flag_ = (merged_flag_ + 1) % 2;

    if (merged_flag_) {
        for (int i = 0; i < iMeta->body().chn_num(); i++) {
            memcpy(&frame_buf_[iMeta->body().frame() * i * 2], &data_in[iMeta->body().frame() * i], iMeta->body().frame() * sizeof(int16_t));
        }

        return true;
    } else {
        for (int i = 0; i < iMeta->body().chn_num(); i++) {
            memcpy(&frame_buf_[iMeta->body().frame() * i * 2 + iMeta->body().frame()], &data_in[iMeta->body().frame() * i], iMeta->body().frame() * sizeof(int16_t));
        }
    }

    // process kws
    thead_kws_run(frame_buf_, frame_buf_len_, temp_buf_, temp_buf_len_);
    thead_kws_postprocess(temp_buf_, temp_buf_len_, output_data_, iMeta->body().chn_num());

    int kws_state = 0;
    int score = 0;

    int first_wkup = 0;

    // check whether the kws is ref chn
    // if (output_data_[ref_chn_id_][1]) {
    //   kws_state = 0;
    // } else {
    // choice one kwsed chn
    for (int i = 0; i < iMeta->body().chn_num(); i++) {
        if (output_data_[i][0] >= 0) {
            LOGD(TAG, "    chn %d wakeup: [%d %d]", i, output_data_[i][0], output_data_[i][1]);

            if (output_data_[i][1] > score) {
                score = output_data_[i][1];
                kws_chn_id_ = i;
            }

            kws_state  = 1;
        }
    }

    // }

    //连续唤醒检查
    if (kws_state == 1 && kws_proc_cnt_ == 0) {
        kws_proc_cnt_  = MIN_WAKE_INTERV;
        max_score_ = score;
        first_wkup = 1;
    } else {
        if (kws_proc_cnt_ > 0) {
            kws_proc_cnt_ --;
        }

        if (max_score_ < score) {
            max_score_ = score;
        } else {
            kws_state = 0;
        }
    }

    //唤醒处理
    if (kws_state == 1 || g_fake_wakeup == 1) {
        if (g_fake_wakeup) {
            first_wkup = 1;
            g_fake_wakeup = 0;
        }

        auto oMeta = std::make_shared<InferOutMessageT>();
        oMeta->body().set_kws_chn(kws_chn_id_);
        oMeta->body().set_kws_id(output_data_[kws_chn_id_][0]);
        oMeta->body().set_kws_score(max_score_);
        oMeta->body().set_first_wakeup(first_wkup);

        auto output = std::make_shared<cx::Buffer>();
        output->SetMetadata("kws_param", oMeta);

        Send(0, output);
        Send(1, output);
    }

    return true;
}

CX_REGISTER_PLUGIN(Inference);

}  // namespace cpt
