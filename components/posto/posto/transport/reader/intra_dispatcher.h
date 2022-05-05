/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_READER_INTRA_DISPATCHER_H_
#define POSTO_TRANSPORT_READER_INTRA_DISPATCHER_H_

#include <functional>
#include <map>
#include <memory>
#include <utility>

#include "posto/base/mutex.h"
#include "posto/common/macros.h"
#include "posto/entity/attributes.h"
#include "posto/transport/reader/listener_handler.h"

namespace posto {
namespace transport {

class IntraDispatcher;
using IntraDispatcherPtr = IntraDispatcher*;

template <typename MessageT>
using IntraListener =
    std::function<void(const std::shared_ptr<MessageT>&)>;

class IntraDispatcher {
 public:
  ~IntraDispatcher();

  template <typename MessageT>
  void OnMessage(const entity::Guid& guid,
                 const std::shared_ptr<MessageT>& message);

  template <typename MessageT>
  void AddListener(const entity::Attributes&, const IntraListener<MessageT>&);
  template <typename MessageT>
  void RemoveListener(const entity::Attributes&);

 private:
  base::mutex mutex_;
  std::map<entity::Guid, ListenerHandlerBasePtr> msg_listeners_;

  POSTO_DECLARE_SINGLETON(IntraDispatcher)
};

template <typename MessageT>
void IntraDispatcher::AddListener(const entity::Attributes& attr,
                                  const IntraListener<MessageT>& listener) {
  base::lock_guard<base::mutex> lock(mutex_);
  auto handler = std::make_shared<ListenerHandler<MessageT>>(listener);
  // replace anyway
  msg_listeners_[attr.guid] = handler;
}

template <typename MessageT>
void IntraDispatcher::RemoveListener(const entity::Attributes& attr) {
  base::lock_guard<base::mutex> lock(mutex_);
  auto it = msg_listeners_.find(attr.guid);
  if (it != msg_listeners_.end()) {
    msg_listeners_.erase(it);
  }
}

template <typename MessageT>
void IntraDispatcher::OnMessage(const entity::Guid& guid,
                                const std::shared_ptr<MessageT>& message) {
  ListenerHandlerBasePtr handler_base;
  {
    base::lock_guard<base::mutex> lock(mutex_);
    {
      auto it = msg_listeners_.find(guid);
      if (it != msg_listeners_.end()) {
        handler_base = it->second;
      }
    }
  }

  if (handler_base) {
    auto handler =
        std::dynamic_pointer_cast<ListenerHandler<MessageT>>(handler_base);
    (*handler)(message);
  }
}

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_READER_INTRA_DISPATCHER_H_
