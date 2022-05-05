/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_PARTICIPANT_H_
#define POSTO_PARTICIPANT_H_

#include <cstdint>
#include <memory>
#include <string>

#include "posto/entity/entity.h"
#include "posto/participant/reader.h"
#include "posto/participant/writer.h"
#include "posto/scheduler/scheduler.h"

namespace posto {

class Domain;

class Participant final : public entity::Entity {
public:
  template <typename MessageT>
  auto CreateReader(const std::string& topic,
                    const MessageCallback<MessageT>& callback,
                    scheduler::Executor* executor = nullptr)
      -> std::shared_ptr<Reader<MessageT>>;

  template <typename MessageT>
  auto CreateWriter(const std::string& topic)
      -> std::shared_ptr<Writer<MessageT>>;

private:
  explicit Participant(const std::string& name);
  uint32_t NewEntityId();

private:
  uint32_t entity_counter_;

  friend class posto::Domain;
};

template <typename MessageT>
auto Participant::CreateReader(const std::string& topic,
                               const MessageCallback<MessageT>& callback,
                               scheduler::Executor* executor)
    -> std::shared_ptr<Reader<MessageT>> {
  std::shared_ptr<Reader<MessageT>> reader =
      std::make_shared<Reader<MessageT>>(attributes(), callback);
  entity::guid::SetEntityId(reader->guid(), NewEntityId());
  reader->attributes().topic = topic;
  if (!reader->Init(executor))
    return nullptr;
  return reader;
}

template <typename MessageT>
auto Participant::CreateWriter(const std::string& topic)
    -> std::shared_ptr<Writer<MessageT>> {
  std::shared_ptr<Writer<MessageT>> writer =
      std::make_shared<Writer<MessageT>>(attributes());
  entity::guid::SetEntityId(writer->guid(), NewEntityId());
  writer->attributes().topic = topic;
  if (!writer->Init())
    return nullptr;
  return writer;
}

}  // namespace posto

#endif  // POSTO_PARTICIPANT_H_
