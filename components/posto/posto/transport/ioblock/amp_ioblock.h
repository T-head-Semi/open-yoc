/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_IOBLOCK_AMPIOBLOCK_H_
#define POSTO_TRANSPORT_IOBLOCK_AMPIOBLOCK_H_

#include "posto/transport/ioblock/ioblock.h"

namespace posto {
namespace transport {

class AmpShm;

class AmpIoBlock : public IoBlock {
 public:
  static bool Init();

  AmpIoBlock();
  explicit AmpIoBlock(size_t size);
  AmpIoBlock(const void* data, size_t size);
  ~AmpIoBlock();

  bool SerializeTo(IoBlockMeta& meta);
  bool DeserializeFrom(const IoBlockMeta& meta);

  void* data() override;
  size_t size() const override;
  size_t block_size() const override;
  uint64_t id() const override;

 private:
  size_t block_size_;
  char* block_addr_;
  size_t size_;
  char* data_;
  AmpShm* shm_;
};

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_IOBLOCK_AMPIOBLOCK_H_
