/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <stdlib.h>

#include <memory>

#include <aos/kernel.h>
#include <ulog/ulog.h>

#include <cxvision/cxvision.h>
#include <alsa/pcm.h>

#include "data_input.h"

#define TAG "data_input"

/* Debug: If equal to 1,pcm simulation mode, at startup time at ignore alsa capture data */
extern int g_pcminput_ignore_alsa;

using DataInputMessageT = posto::Message<thead::voice::proto::DataInputMsg>;
using RecordMessageT    = posto::Message<thead::voice::proto::RecordMsg>;

namespace cpt
{

class DataInput : public cx::PluginBase
{
public:
    DataInput();
    static void data_input(void *arg);

    bool Init(const std::map<std::string, std::string> &props) override;
    bool DeInit() override;
    bool Process(const std::vector<cx::BufferPtr> &data_vec) override
    {
        return true;
    }
private:
    aos_task_t task_;

    aos_pcm_t *pcmC0_;
    aos_pcm_t *pcmC1_;

    aos_pcm_t *capture_init(const char *devname, unsigned int rate, int channel, int format, int peroid_ms);
    int capture(void *data, int len);
    void config(const std::map<std::string, std::string> &props);

    std::shared_ptr<posto::Participant> participant_;
    std::shared_ptr<posto::Reader<RecordMessageT>> reader_;

    bool start_record_;

    int chn_num;
    int interleaved;
    int format; /* example:16 bit, 24 bit */
    int sample_rate;
    int frame_ms;
};

DataInput::DataInput()
{
    pcmC0_ = pcmC1_ = NULL;
    start_record_ = false;
    task_.hdl = NULL;
}

aos_pcm_t *DataInput::capture_init(const char *devname, unsigned int rate /*16000*/, int channel,
                                   int format /*16bit*/, aos_pcm_uframes_t peroid_size /*10ms*/)
{
    aos_pcm_hw_params_t *params;
    aos_pcm_t           *pcm = NULL;

    int err = 0;

    err = aos_pcm_open(&pcm, devname, AOS_PCM_STREAM_CAPTURE, 0);

    if (err < 0) {
        LOGE(TAG, "aos_pcm_open %s error", devname);
        return NULL;
    }

    aos_pcm_hw_params_alloca(&params);
    err = aos_pcm_hw_params_any(pcm, params);

    if (err < 0) {
        LOGE(TAG, "Broken configuration for this PCM: no configurations available");
        aos_pcm_close(pcm);
        return NULL;
    }

    err = aos_pcm_hw_params_set_access(pcm, params, AOS_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        LOGE(TAG, "Access type not available");
        aos_pcm_close(pcm);
        return NULL;
    }

    err = aos_pcm_hw_params_set_format(pcm, params, format);

    if (err < 0) {
        LOGE(TAG, "Sample format non available");
        aos_pcm_close(pcm);
        return NULL;
    }

    err = aos_pcm_hw_params_set_channels(pcm, params, channel);

    if (err < 0) {
        LOGE(TAG, "Channels count non available");
        aos_pcm_close(pcm);
        return NULL;
    }

    aos_pcm_hw_params_set_rate_near(pcm, params, &rate, 0);

    aos_pcm_uframes_t val_peroid_size = peroid_size;
    aos_pcm_hw_params_set_period_size_near(pcm, params, &val_peroid_size, 0);

    aos_pcm_uframes_t val_buffer_frames = val_peroid_size * 16; /*buffer保存16个frame*/
    aos_pcm_hw_params_set_buffer_size_near(pcm, params, &val_buffer_frames);

    err = aos_pcm_hw_params(pcm, params);

    if (err < 0) {
        LOGE(TAG, "aos_pcm_hw_params error");
        aos_pcm_close(pcm);
        return NULL;
    }

    return pcm;
}

/**
 * @brief  capture audio data from alsa 
 * The current code is aimed at two D1 sound cards. 
 * pcmC0：Digital microphone,3 channel: dmic1 dmic2 dmic3
 * pcmC1：Analog microphone,3 channel: ref1 ref2 hamic(headphone mic)
 *
 * pcmC0 & pcmC1：dmic1 dmic2 ref1
 * only pcmC0：dmic1 dmic2 zero
 * only pcmC1：hamic hamic ref1
 * 
 * @param  [out] data : 3-channel interleaved audio (mic1,mic2,ref)
 * @param  [int] len : data buffer byte length
 * @return <0 failed, >0 byte length of capture data
 */
int DataInput::capture(void *data, int len)
{
    int     rlen = 0, reflen = len;
    int8_t *dataref = NULL;

    /* pcmC0 & pcmC1 */
    if (pcmC0_ && pcmC1_) {
        aos_pcm_wait(pcmC0_, AOS_WAIT_FOREVER);
        rlen = aos_pcm_readi(pcmC0_, (void *)data, aos_pcm_bytes_to_frames(pcmC0_, len));
        rlen = aos_pcm_frames_to_bytes(pcmC0_, rlen);

        dataref = (int8_t *)malloc(len);

        aos_pcm_wait(pcmC1_, AOS_WAIT_FOREVER);
        int rlenc1 = aos_pcm_readi(pcmC1_, (void *)dataref, aos_pcm_bytes_to_frames(pcmC1_, reflen));
        rlenc1 = aos_pcm_frames_to_bytes(pcmC1_, rlenc1);

        if (rlen != rlenc1) {
            LOGW(TAG, "capture %d %d", rlen, reflen);
            return 0;
        }

        if (rlen != len) {
            LOGW(TAG, "capture %d", rlen);
            return 0;
        }
    }

    /* Only pcmC0 */
    if (pcmC0_ && pcmC1_ == NULL) {
        aos_pcm_wait(pcmC0_, AOS_WAIT_FOREVER);
        rlen = aos_pcm_readi(pcmC0_, (void *)data, aos_pcm_bytes_to_frames(pcmC0_, len));
        rlen = aos_pcm_frames_to_bytes(pcmC0_, rlen);

        int16_t *ptr = (int16_t *)data;

        for (int i = 0; i < rlen / 2; i += 3) {
            ptr[2] = 0; /* channel 3 set zero */
            ptr += 3;
        }
    }

    /* Only pcmC1: channel: ref1 ref2 hamic*/
    if (pcmC0_ == NULL && pcmC1_) {
        aos_pcm_wait(pcmC1_, AOS_WAIT_FOREVER);
        rlen = aos_pcm_readi(pcmC1_, (void *)data, aos_pcm_bytes_to_frames(pcmC1_, len));
        rlen = aos_pcm_frames_to_bytes(pcmC1_, rlen);

        int16_t *ptr = (int16_t *)data;

        for (int i = 0; i < rlen / 2; i += 3) {
            ptr[1] = ptr[2];
            ptr[2] = ptr[0];
            ptr[0] = ptr[1];
            ptr += 3;
        }
    }

    /* pcm push hook, overwrite capture data */
    if (g_pcminput_ignore_alsa == 0) {
        pcm_hook_call(data, rlen);
    } else {
        /* no alsa data to alg when dev startup */
        int hook_ret = pcm_hook_call(data, rlen);

        if (hook_ret <= 0) {
            rlen = hook_ret;
        }
    }

    /* pcmC0 & pcmC1: channel 3 import reference sound*/
    if (pcmC0_ && pcmC1_) {
        if (dataref) {
            int16_t *pmic = (int16_t *)data;
            int16_t *pref = (int16_t *)dataref;

            for (int i = 0; i < rlen / 2; i += 3) {
                pmic[2] = pref[0];
                pmic += 3;
                pref += 3;
            }

            free(dataref);
        }
    }

    return rlen;
}

void DataInput::data_input(void *arg)
{

    DataInput *self         = static_cast<DataInput *>(arg);
    ssize_t    capture_byte = 0;
    int16_t   *capture_buf  = NULL;

    self->pcmC0_ = self->pcmC1_ = NULL;

    //jtag debug
    //aos_msleep(3000);

    /* Single frame single channel sample count */
    aos_pcm_uframes_t peroid_size = self->frame_ms * (self->sample_rate / 1000); 

    /* init sound car, peroid_size / 2 reduce driver latency */
#ifndef DISABLED_PCMC0
    self->pcmC0_ = self->capture_init("pcmC0", self->sample_rate, self->chn_num, self->format, peroid_size / 2);
#endif

    if (self->pcmC0_) {
        capture_byte = aos_pcm_frames_to_bytes(self->pcmC0_, peroid_size);
        capture_buf  = (int16_t *)malloc(capture_byte);
    } else {
        LOGW(TAG, "Init device pcmC0 error");
    }

    self->pcmC1_ = self->capture_init("pcmC1", self->sample_rate, self->chn_num, self->format,
                                      peroid_size / 2);

    if (self->pcmC1_) {
        if (self->pcmC0_ == NULL) {
            capture_byte = aos_pcm_frames_to_bytes(self->pcmC1_, peroid_size);
            capture_buf  = (int16_t *)malloc(capture_byte);
        }
    } else {
        LOGW(TAG, "Init device pcmC1 error");
    }

    /* start capture device */
    if (self->pcmC0_) {
        aos_pcm_readi(self->pcmC0_, (void *)capture_buf, 1);
    }

    if (self->pcmC1_) {
        aos_pcm_readi(self->pcmC1_, (void *)capture_buf, 1);
    }

    LOGD(TAG, "Ignore initial packets");

    for (int i = 0; i < 1200; i++) {
        self->capture(capture_buf, capture_byte / 4);
    }

    LOGD(TAG, "Go to the algorithm process");

    /* capture main loop */
    while (1) {
        int rlen = self->capture(capture_buf, capture_byte);

        if (rlen <= 0) {
            continue;
        }

        /* prepare publish data */
        auto     oMemory = cx::MemoryHelper::Malloc(capture_byte);
        int16_t *data    = (int16_t *)oMemory->data();

        /* data interleaved check */
        if (self->interleaved == 0) {
            for (int j = 0; j < peroid_size; j++) {
                for (int i = 0; i < self->chn_num; i++) {
                    data[peroid_size * i + j] = capture_buf[self->chn_num * j + i];
                }
            }
        }

        /* publish node data */
        auto oMeta         = std::make_shared<DataInputMessageT>();
        oMeta->body().set_chn_num(self->chn_num);
        oMeta->body().set_format(self->format);
        oMeta->body().set_sample_rate(self->sample_rate);
        oMeta->body().set_frame(peroid_size);

        auto output = std::make_shared<cx::Buffer>();
        output->AddMemory(oMemory);
        output->SetMetadata("alsa_param", oMeta);

        self->Send(0, output);

        if (self->start_record_ == true) {
            self->Send(1, output);
        }
    }

    aos_task_exit(0);
}

void DataInput::config(const std::map<std::string, std::string> &props)
{
    for (auto iter = props.begin(); iter != props.end(); ++iter) {
        if (strcmp(iter->first.c_str(), "chn_num") == 0) {
            this->chn_num = atoi(iter->second.c_str());
        } else if (strcmp(iter->first.c_str(), "frame_ms") == 0) {
            this->frame_ms = atoi(iter->second.c_str());
        } else if (strcmp(iter->first.c_str(), "interleaved") == 0) {
            this->interleaved = atoi(iter->second.c_str());
        } else if (strcmp(iter->first.c_str(), "sample_rate") == 0) {
            this->sample_rate = atoi(iter->second.c_str());
        } else if (strcmp(iter->first.c_str(), "pcm_bits") == 0) {
            this->format = atoi(iter->second.c_str());
        }
    }

    LOGI(TAG, "chn_num %d, frame_ms %d, rate %d, bits %d, interleaved %d",
         this->chn_num, this->frame_ms, this->sample_rate, this->format,
         this->interleaved);
}

bool DataInput::Init(const std::map<std::string, std::string> &props)
{
    //param config
    config(props);

    start_record_ = false;

    participant_ = posto::Domain::CreateParticipant("cmd_consumer");

    reader_ = participant_->CreateReader<RecordMessageT>("RecordMsg",
    [this](const std::shared_ptr<RecordMessageT> &msg) {

        // LOGD(TAG, "Message got, cmd_id: %d\n", msg->body().cmd_id());
        switch (msg->body().cmd()) {
            case thead::voice::proto::START:
                start_record_ = true;
                break;

            case thead::voice::proto::STOP:
                start_record_ = false;
                break;

            default:
                break;
        }
    });

    aos_task_new_ext(&task_, "PcmInput", &data_input, this, 1024 * 8, AOS_DEFAULT_APP_PRI - 4);
    return true;
}

bool DataInput::DeInit()
{
    return true;
}

CX_REGISTER_PLUGIN(DataInput);

} // namespace cpt
