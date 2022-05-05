#ifndef CXVISION_BASE_MEMORY_H_
#define CXVISION_BASE_MEMORY_H_

#include <cstddef>

#include <posto/posto.h>

namespace cx {

using MemoryPtr = posto::transport::IoBlockPtr;

class MemoryHelper {
public:
  static MemoryPtr Malloc(size_t size);
  static MemoryPtr MallocAndCopy(const void* data, size_t size);
};

} // namespace cx

#endif  // CXVISION_BASE_MEMORY_H_
