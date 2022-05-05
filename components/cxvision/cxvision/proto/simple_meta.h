/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <cstring>
#include <string>

#include <posto/posto.h>

namespace cx {
namespace proto {

class SimpleMeta : public posto::Serializable {
public:
  size_t ByteSizeLong() const override {
    return value_.size();
  }

  bool SerializeToArray(void* data, size_t size) const override {
    std::memcpy(data, value_.data(), value_.size());
    return true;
  }

  bool ParseFromArray(const void* data, size_t size) {
    value_.resize(size);
    std::memcpy(&value_[0], data, size);
    return true;
  }

  void set_value(const std::string& value) {
    value_ = value;
  }

  const std::string& value() const {
    return value_;
  }

private:
  std::string value_;
};

}  // namespace proto
}  // namespace cx
