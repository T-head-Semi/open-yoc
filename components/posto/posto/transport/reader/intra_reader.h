/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_READER_INTRA_READER_H_
#define POSTO_TRANSPORT_READER_INTRA_READER_H_

#include "posto/transport/reader/intra_dispatcher.h"

namespace posto {
namespace transport {

template <typename MessageT>
class IntraReader {
 public:
  using MessagePtr = std::shared_ptr<MessageT>;
  using MessageCallback = std::function<void(const MessagePtr&,
      const entity::Attributes&)>;

  IntraReader(const entity::Attributes& attr,
              const MessageCallback& callback);
  ~IntraReader();
  void OnNewMessage(const MessagePtr& msg);

 private:
  entity::Attributes attr_;
  MessageCallback msg_callback_;
  IntraDispatcherPtr dispatcher_;
};

template <typename MessageT>
IntraReader<MessageT>::IntraReader(const entity::Attributes& attr,
    const MessageCallback& callback)
    : attr_(attr), msg_callback_(callback) {
  dispatcher_ = IntraDispatcher::Instance();
  dispatcher_->AddListener<MessageT>(attr_,
      std::bind(&IntraReader<MessageT>::OnNewMessage, this,
          std::placeholders::_1));
}

template <typename MessageT>
IntraReader<MessageT>::~IntraReader() {
  dispatcher_->RemoveListener<MessageT>(attr_);
}

template <typename MessageT>
void IntraReader<MessageT>::OnNewMessage(const MessagePtr& msg) {
  if (msg_callback_ != nullptr) {
    msg_callback_(msg, attr_);
  }
}

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_READER_INTRA_READER_H_
