/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_PARTICIPANT_READER_H_
#define POSTO_PARTICIPANT_READER_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "posto/discovery/endpoint_manager.h"
#include "posto/entity/endpoint.h"
#include "posto/scheduler/scheduler.h"
#include "posto/profile/statistics.h"
#include "posto/transport/reader/reader.h"

namespace posto {

template <typename MessageT>
using MessageCallback = std::function<void(const std::shared_ptr<MessageT>&)>;

template <typename MessageT>
class Reader : public entity::Endpoint {
public:
  using TransportReaderPtr =
      std::shared_ptr<transport::Reader<MessageT>>;

  Reader(const entity::Attributes& attr,
         const MessageCallback<MessageT>& callback);
  ~Reader();

  bool Init(scheduler::Executor* executor);

  profile::Statistics* GetStatistics();

private:
  TransportReaderPtr transport_reader_;
  MessageCallback<MessageT> callback_;
  discovery::EndpointManagerPtr endpoint_manager_;
  scheduler::Task* task_;
};

template <typename MessageT>
Reader<MessageT>::Reader(const entity::Attributes& attr,
                         const MessageCallback<MessageT>& callback)
  : callback_(callback) {
    attr_ = attr;
}

template <typename MessageT>
Reader<MessageT>::~Reader() {
  scheduler::Scheduler::Instance()->DestroyTask(task_);
  endpoint_manager_->Leave(attributes(), ENTITY_READER);
}

template <typename MessageT>
bool Reader<MessageT>::Init(scheduler::Executor* executor) {
  auto sched = scheduler::Scheduler::Instance();
  task_ = sched->CreateTask([this](const std::shared_ptr<void>& msg) {
    this->callback_(std::static_pointer_cast<MessageT>(msg));
  }, executor);
  if (!task_) {
    return false;
  }
  transport_reader_ =
      std::make_shared<transport::Reader<MessageT>>(attributes(),
          [this, sched](const std::shared_ptr<MessageT>& msg,
                 const entity::Attributes& attr) {
        (void)attr;
        this->task_->Enqueue(msg);
      });
  endpoint_manager_ = discovery::EndpointManager::Instance();
  return endpoint_manager_->Join(attributes(), ENTITY_READER);
}

template <typename MessageT>
profile::Statistics* Reader<MessageT>::GetStatistics() {
  return task_->GetStatistics();
}

}  // namespace posto

#endif  // POSTO_PARTICIPANT_READER_H_
