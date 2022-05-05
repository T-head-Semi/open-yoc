/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_IOBLOCK_IOBLOCK_H_
#define POSTO_TRANSPORT_IOBLOCK_IOBLOCK_H_

#include <cstddef>
#include <cstdint>
#include <memory>

namespace posto {

bool Init();

namespace transport {

class IoBlock;
using IoBlockPtr = std::shared_ptr<IoBlock>;

struct IoBlockMeta {
  // address for AMP ROTS or fd for Linux
  uint64_t id;
  uint32_t data_size;
  uint32_t block_size;
};

static_assert(sizeof(IoBlockMeta) == 16, "IoBlockMeta size error");

class IoBlock {
 public:
  static IoBlockPtr New(size_t size);
  static IoBlockPtr Fill(const void* data, size_t size);

  virtual ~IoBlock();

  // Get pointer of payload for r/w
  virtual void* data() = 0;
  // Get size of payload
  virtual size_t size() const = 0;
  // Size of block
  virtual size_t block_size() const = 0;
  // ID of block
  virtual uint64_t id() const = 0;

 private:
  static bool Init();

 friend bool posto::Init();
};

} // namespace transport
} // namespace posto

#endif  // POSTO_TRANSPORT_IOBLOCK_IOBLOCK_H_
