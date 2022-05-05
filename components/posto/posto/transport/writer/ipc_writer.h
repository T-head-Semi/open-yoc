/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_WRITER_IPC_WRITER_H_
#define POSTO_TRANSPORT_WRITER_IPC_WRITER_H_

#include <cstring>
#include <memory>
#include <string>

#include "posto/transport/amp/amp_ipc.h"

namespace posto {
namespace transport {

template <typename MessageT>
class IpcWriter {
public:
  using MessagePtr = std::shared_ptr<MessageT>;

  explicit IpcWriter(const entity::Attributes& attr);
  virtual ~IpcWriter();

  bool Write(const MessagePtr& msg,
             const entity::Attributes& dst_attr);

private:
  entity::Attributes attr_;
  AmpIpc* amp_ipc_;
};

template <typename MessageT>
IpcWriter<MessageT>::IpcWriter(const entity::Attributes& attr)
    : attr_(attr), amp_ipc_(AmpIpc::Instance()) {}

template <typename MessageT>
IpcWriter<MessageT>::~IpcWriter() {}

template <typename MessageT>
bool IpcWriter<MessageT>::Write(const MessagePtr& msg,
                                const entity::Attributes& dst_attr) {
  msg->header().dst_id = dst_attr.guid;
  std::string str{};
  size_t total_size = msg->ByteSizeLong();
  str.resize(total_size);
  char *_buf = &str[0];
  msg->SerializeToArray(_buf, total_size);
  amp_ipc_->SendTo((uint8_t*)str.data(), str.size(),
      (uint32_t)dst_attr.host_id, AmpIpc::USER_MSG_PORT);
  return true;
}

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_WRITER_IPC_WRITER_H_
