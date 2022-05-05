/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef CXVISION_PLUGIN_BUFFER_H_
#define CXVISION_PLUGIN_BUFFER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <posto/posto.h>

#include "cxvision/base/memory.h"

namespace cx {

class Buffer;
using BufferPtr = std::shared_ptr<Buffer>;

class Buffer final : public posto::MessageBase {
public:
  bool AddMemory(const cx::MemoryPtr& memory) {
    memories_.push_back(memory);
    return true;
  }

  cx::MemoryPtr GetMemory(unsigned int index) {
    if (index < memories_.size()) {
      return memories_.at(index);
    }
    return nullptr;
  }

  bool SetMetadata(const std::string& key, std::shared_ptr<void> meta) {
    metas_[key] = meta;
    return true;
  }

  template <typename T>
  std::shared_ptr<T> GetMetadata(const std::string& key) {
    const auto& it = metas_.find(key);
    if (it != metas_.end()) {
      return std::static_pointer_cast<T>(it->second);
    }
    return nullptr;
  }

  size_t _ByteSizeLong() const override;
  bool _SerializeToArray(void* data, size_t size) const override;
  bool _ParseFromArray(const void* data, size_t size) override;

private:
  std::vector<cx::MemoryPtr> memories_;
  std::map<std::string, std::shared_ptr<void>> metas_;
};

}  // namespace cx

#endif  // CXVISION_PLUGIN_BUFFER_H_
