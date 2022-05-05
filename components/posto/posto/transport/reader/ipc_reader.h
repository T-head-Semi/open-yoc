/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_READER_IPC_READER_H_
#define POSTO_TRANSPORT_READER_IPC_READER_H_

#include <memory>

#include "posto/message/message.h"
#include "posto/transport/reader/ipc_dispatcher.h"

namespace posto {
namespace transport {

template <typename MessageT>
class IpcReader {
 public:
  using MessagePtr = std::shared_ptr<MessageT>;
  using MessageCallback = std::function<void(const MessagePtr&,
      const entity::Attributes&)>;

  IpcReader(const entity::Attributes& attr,
            const MessageCallback& callback);
  ~IpcReader();
  void OnNewMessage(uint8_t *data, size_t size);

 private:
  entity::Attributes attr_;
  MessageCallback msg_callback_;
  IpcDispatcherPtr dispatcher_;
};

template <typename MessageT>
IpcReader<MessageT>::IpcReader(const entity::Attributes& attr,
    const MessageCallback& callback)
    : attr_(attr), msg_callback_(callback) {
  dispatcher_ = IpcDispatcher::Instance();
  dispatcher_->AddListener(attr_,
      std::bind(&IpcReader<MessageT>::OnNewMessage, this,
          std::placeholders::_1, std::placeholders::_2));
}

template <typename MessageT>
IpcReader<MessageT>::~IpcReader() {
  dispatcher_->RemoveListener(attr_);
}

template <typename MessageT>
void IpcReader<MessageT>::OnNewMessage(uint8_t *data, size_t size) {
  auto msg = std::make_shared<MessageT>();
  msg->ParseFromArray(data, size);
  if (msg_callback_ != nullptr) {
    msg_callback_(msg, attr_);
  }
}

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_READER_IPC_READER_H_
