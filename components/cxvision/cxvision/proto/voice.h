/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <cstdint>
#include <string>

#include <posto/posto.h>

#define CX_HTONL(n) (((((uint32_t)(n) & 0xFF)) << 24) |     \
                    ((((uint32_t)(n) & 0xFF00)) << 8) |     \
                    ((((uint32_t)(n) & 0xFF0000)) >> 8) |   \
                    ((((uint32_t)(n) & 0xFF000000)) >> 24))

#define CX_NTOHL(n) (((((uint32_t)(n) & 0xFF)) << 24) |     \
                    ((((uint32_t)(n) & 0xFF00)) << 8) |     \
                    ((((uint32_t)(n) & 0xFF0000)) >> 8) |   \
                    ((((uint32_t)(n) & 0xFF000000)) >> 24))

namespace thead {
namespace voice {
namespace proto {

/* session command message */
enum SessionCmd : int32_t {
  BEGIN = 0,
  END = 1,
  TIMEOUT = 2,
};

class SessionMsg final : public posto::Serializable {
public:
  size_t ByteSizeLong() const override {
    return (4 * sizeof(int32_t) + kws_word_.size());
  }

  bool SerializeToArray(void* data, size_t size) const override {
    uint32_t* ptr = (uint32_t*)data;
    *ptr = CX_HTONL(cmd_id_);
    *(ptr + 1) = CX_HTONL(kws_id_);
    *(ptr + 2) = CX_HTONL(kws_score_);
    *(ptr + 3) = CX_HTONL(kws_word_.size());
    std::memcpy(ptr + 4, kws_word_.data(), kws_word_.size());
    return true;
  }

  bool ParseFromArray(const void* data, size_t size) {
    uint32_t* ptr = (uint32_t*)data;
    cmd_id_ = (::thead::voice::proto::SessionCmd)CX_NTOHL(*ptr);
    kws_id_ = (int32_t)CX_NTOHL(*(ptr + 1));
    kws_score_ = (int32_t)CX_NTOHL(*(ptr + 2));
    size_t word_size = CX_NTOHL(*(ptr + 3));
    kws_word_.resize(word_size);
    std::memcpy(&kws_word_[0], ptr + 4, word_size);
    return true;
  }

  ::thead::voice::proto::SessionCmd cmd_id() const {
    return cmd_id_;
  }

  int32_t kws_id() const {
    return kws_id_;
  }

  int32_t kws_score() const {
    return kws_score_;
  }

  const std::string& kws_word() const {
    return kws_word_;
  }

  void set_cmd_id(::thead::voice::proto::SessionCmd value) {
    cmd_id_ = value;
  }

  void set_kws_id(int32_t value) {
    kws_id_ = value;
  }

  void set_kws_score(int32_t value) {
    kws_score_ = value;
  }

  void set_kws_word(const std::string& value) {
    kws_word_ = value;
  }

private:
  ::thead::voice::proto::SessionCmd cmd_id_;
  int32_t kws_id_;
  int32_t kws_score_;
  std::string kws_word_;
};

/* record command message */
enum RecordCmd : int32_t {
  START = 0,
  STOP = 1,
};

class RecordMsg final : public posto::Serializable {
public:
  size_t ByteSizeLong() const override {
    return sizeof(int32_t);
  }

  bool SerializeToArray(void* data, size_t size) const override {
    uint32_t* ptr = (uint32_t*)data;
    *ptr = CX_HTONL(cmd_);
    return true;
  }

  bool ParseFromArray(const void* data, size_t size) {
    uint32_t* ptr = (uint32_t*)data;
    cmd_ = (::thead::voice::proto::RecordCmd)CX_NTOHL(*ptr);
    return true;
  }

  void set_cmd(::thead::voice::proto::RecordCmd value) {
    cmd_ = value;
  }

  ::thead::voice::proto::RecordCmd cmd() const {
    return cmd_;
  }

private:
  ::thead::voice::proto::RecordCmd cmd_;
};

/* Vad output message */
class VadOutMsg final : public posto::Serializable {
public:
  size_t ByteSizeLong() const override {
    return sizeof(int32_t);
  }

  bool SerializeToArray(void* data, size_t size) const override {
    uint32_t* ptr = (uint32_t*)data;
    *ptr = CX_HTONL(vad_status_);
    return true;
  }

  bool ParseFromArray(const void* data, size_t size) {
    uint32_t* ptr = (uint32_t*)data;
    vad_status_ = (int32_t)CX_NTOHL(*ptr);
    return true;
  }

  void set_vad_status(int32_t value) {
    vad_status_ = value;
  }

  int vad_status() const {
    return vad_status_;
  }

private:
  int32_t vad_status_;
};

/* inference output message */
class InferOutMsg final : public posto::Serializable {
public:
  size_t ByteSizeLong() const override {
    return (4 * sizeof(int32_t));
  }

  bool SerializeToArray(void* data, size_t size) const override {
    uint32_t* ptr = (uint32_t*)data;
    *ptr       = CX_HTONL(kws_chn_);
    *(ptr + 1) = CX_HTONL(kws_id_);
    *(ptr + 2) = CX_HTONL(kws_score_);
    *(ptr + 3) = CX_HTONL(first_wakeup_);
    return true;
  }

  bool ParseFromArray(const void* data, size_t size) {
    uint32_t* ptr = (uint32_t*)data;
    kws_chn_      = (int32_t)CX_NTOHL(*ptr);
    kws_id_       = (int32_t)CX_NTOHL(*(ptr + 1));
    kws_score_    = (int32_t)CX_NTOHL(*(ptr + 2));
    first_wakeup_ = (int32_t)CX_NTOHL(*(ptr + 3));
    return true;
  }

  int32_t first_wakeup() const {
    return first_wakeup_;
  }

  int32_t kws_score() const {
    return kws_score_;
  }

  int32_t kws_id() const {
    return kws_id_;
  }

  int32_t kws_chn() const {
    return kws_chn_;
  }

  void set_first_wakeup(int32_t value) {
    first_wakeup_ = value;
  }

  void set_kws_score(int32_t value) {
    kws_score_ = value;
  }

  void set_kws_id(int32_t value) {
    kws_id_ = value;
  }

  void set_kws_chn(int32_t value) {
    kws_chn_ = value;
  }

private:
    int32_t kws_chn_;
    int32_t kws_id_;
    int32_t kws_score_;
    int32_t first_wakeup_;
};


/* SSP output message */
class SspOutMsg final : public posto::Serializable {
public:
  size_t ByteSizeLong() const override {
    return (4 * sizeof(int32_t));
  }

  bool SerializeToArray(void* data, size_t size) const override {
    uint32_t* ptr = (uint32_t*)data;
    *ptr       = CX_HTONL(vad_res_);
    *(ptr + 1) = CX_HTONL(chn_num_);
    *(ptr + 2) = CX_HTONL(sample_rate_);
    *(ptr + 3) = CX_HTONL(frame_);
    return true;
  }

  bool ParseFromArray(const void* data, size_t size) {
    uint32_t* ptr = (uint32_t*)data;
    vad_res_      = (int32_t)CX_NTOHL(*ptr);
    chn_num_      = (int32_t)CX_NTOHL(*(ptr + 1));
    sample_rate_  = (int32_t)CX_NTOHL(*(ptr + 2));
    frame_        = (int32_t)CX_NTOHL(*(ptr + 3));
    return true;
  }

  int32_t vad_res() const {
    return vad_res_;
  }

  int32_t chn_num() const {
    return chn_num_;
  }

  int32_t sample_rate() const {
    return sample_rate_;
  }

  int32_t frame() const {
    return frame_;
  }

  void set_vad_res(int32_t value) {
    vad_res_ = value;
  }

  void set_chn_num(int32_t value) {
    chn_num_ = value;
  }

  void set_sample_rate(int32_t value) {
    sample_rate_ = value;
  }

  void set_frame(int32_t value) {
    frame_ = value;
  }

private:
    int32_t vad_res_;
    int32_t chn_num_;
    int32_t sample_rate_;
    int32_t frame_;
};


/* data input message */
class DataInputMsg final : public posto::Serializable {
public:
  size_t ByteSizeLong() const override {
    return (4 * sizeof(int32_t));
  }

  bool SerializeToArray(void* data, size_t size) const override {
    uint32_t* ptr = (uint32_t*)data;
    *ptr       = CX_HTONL(chn_num_);
    *(ptr + 1) = CX_HTONL(format_);
    *(ptr + 2) = CX_HTONL(sample_rate_);
    *(ptr + 3) = CX_HTONL(frame_);
    return true;
  }

  bool ParseFromArray(const void* data, size_t size) {
    uint32_t* ptr = (uint32_t*)data;
    chn_num_      = (int32_t)CX_NTOHL(*ptr);
    format_       = (int32_t)CX_NTOHL(*(ptr + 1));
    sample_rate_  = (int32_t)CX_NTOHL(*(ptr + 2));
    frame_        = (int32_t)CX_NTOHL(*(ptr + 3));
    return true;
  }

  int32_t format() const {
    return format_;
  }

  int32_t chn_num() const {
    return chn_num_;
  }

  int32_t sample_rate() const {
    return sample_rate_;
  }

  int32_t frame() const {
    return frame_;
  }

  void set_format(int32_t value) {
    format_ = value;
  }

  void set_chn_num(int32_t value) {
    chn_num_ = value;
  }

  void set_sample_rate(int32_t value) {
    sample_rate_ = value;
  }

  void set_frame(int32_t value) {
    frame_ = value;
  }

private:
  int32_t chn_num_;
  int32_t format_;
  int32_t sample_rate_;
  int32_t frame_;
};

}  // namespace proto
}  // namespace voice
}  // namespace thead
