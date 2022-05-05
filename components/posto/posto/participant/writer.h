/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_PARTICIPANT_WRITER_H_
#define POSTO_PARTICIPANT_WRITER_H_

#include <cstddef>
#include <memory>
#include <string>

#include "posto/discovery/endpoint_manager.h"
#include "posto/entity/endpoint.h"
#include "posto/profile/statistics.h"
#include "posto/transport/writer/writer.h"

namespace posto {

template <typename MessageT>
class Writer : public entity::Endpoint {
public:
  using TransportWriterPtr =
      std::shared_ptr<transport::Writer<MessageT>>;

  explicit Writer(const entity::Attributes& attr);
  ~Writer();

  bool Init();
  bool Write(const std::shared_ptr<MessageT>& msg_ptr);

  profile::Statistics* GetStatistics();

private:
  TransportWriterPtr transport_writer_;
  discovery::EndpointManagerPtr endpoint_manager_;
};

template <typename MessageT>
Writer<MessageT>::Writer(const entity::Attributes& attr) {
  attr_ = attr;
}

template <typename MessageT>
Writer<MessageT>::~Writer() {
  endpoint_manager_->Leave(attributes(), ENTITY_WRITER);
}

template <typename MessageT>
bool Writer<MessageT>::Init() {
  transport_writer_ =
      std::make_shared<transport::Writer<MessageT>>(attributes());
  endpoint_manager_ = discovery::EndpointManager::Instance();
  return endpoint_manager_->Join(attributes(), ENTITY_WRITER);
}

template <typename MessageT>
bool Writer<MessageT>::Write(const std::shared_ptr<MessageT>& msg_ptr) {
  return transport_writer_->Write(msg_ptr);
}

template <typename MessageT>
profile::Statistics* Writer<MessageT>::GetStatistics() {
  return transport_writer_->GetStatistics();
}

}  // namespace posto

#endif  // POSTO_PARTICIPANT_WRITER_H_
