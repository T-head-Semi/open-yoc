/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_MESSAGE_MESSAGE_H_
#define POSTO_MESSAGE_MESSAGE_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "posto/entity/guid.h"
#include "posto/transport/ioblock/ioblock.h"

namespace posto {

struct MessageHeader {
  uint64_t sequence;
  uint64_t timestamp;
  entity::Guid dst_id;
};

static_assert(sizeof(MessageHeader) == 32, "MessageHeader size error");

class MessageBase {
public:
  static constexpr size_t kMaxBlockSize = 8;

  MessageBase() {}
  virtual ~MessageBase() {}

  virtual size_t _ByteSizeLong() const = 0;
  virtual bool _SerializeToArray(void* data, size_t size) const = 0;
  virtual bool _ParseFromArray(const void* data, size_t size) = 0;

  size_t ByteSizeLong() const;
  bool SerializeToArray(void* data, size_t size) const;
  bool ParseFromArray(const void* data, size_t size);

  bool AppendBlock(const transport::IoBlockPtr& _block) {
    if (io_blocks_.size() < kMaxBlockSize) {
      io_blocks_.emplace_back(_block);
      return true;
    }
    return false;
  }

  const std::vector<transport::IoBlockPtr>& io_blocks() {
    return io_blocks_;
  }

  MessageHeader& header() { return header_; }
  const MessageHeader& header() const { return header_; }

private:
  MessageHeader header_;
  std::vector<transport::IoBlockPtr> io_blocks_;
};

template <typename UserT>
class Message final : public MessageBase {
public:
  Message() : body_(new UserT()) {}
  ~Message() {
    if (body_) { delete body_; }
  }

  size_t _ByteSizeLong() const override {
    return body().ByteSizeLong();
  }

  bool _SerializeToArray(void* data, size_t size) const override {
    return body().SerializeToArray(data, size);
  }

  bool _ParseFromArray(const void* data, size_t size) override {
    return body().ParseFromArray(data, size);
  }

  inline UserT& body() { return *body_; }
  inline const UserT& body() const { return *body_; }

private:
  UserT* body_;
};

class Serializable {
public:
  virtual ~Serializable() {}
  virtual size_t ByteSizeLong() const = 0;
  virtual bool SerializeToArray(void* data, size_t size) const = 0;
  virtual bool ParseFromArray(const void* data, size_t size) = 0;
};

}  // namespace posto

#endif // POSTO_MESSAGE_MESSAGE_H_
