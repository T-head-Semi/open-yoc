/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_LISTENER_HANDLER_H_
#define POSTO_TRANSPORT_LISTENER_HANDLER_H_

#include <functional>
#include <memory>

namespace posto {
namespace transport {

class ListenerHandlerBase {
 public:
  virtual ~ListenerHandlerBase() {}
};

using ListenerHandlerBasePtr = std::shared_ptr<ListenerHandlerBase>;

template <typename MessageT>
class ListenerHandler final : public ListenerHandlerBase {
 public:
  using MessagePtr = std::shared_ptr<MessageT>;
  using Listener = std::function<void(const MessagePtr&)>;

  explicit ListenerHandler(const Listener& listener) : listener_(listener) {}

  void operator()(const MessagePtr& msg) {
    listener_(msg);
  }

 private:
  Listener listener_;
};

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_LISTENER_HANDLER_H_
