#include "cxvision/base/memory.h"

namespace cx {

// static
MemoryPtr MemoryHelper::Malloc(size_t size) {
  return posto::transport::IoBlock::New(size);
}

// static
MemoryPtr MemoryHelper::MallocAndCopy(const void* data, size_t size) {
  return posto::transport::IoBlock::Fill(data, size);
}

} // namespace cx
