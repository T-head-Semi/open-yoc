/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_ENTITY_ATTRIBUTES_H_
#define POSTO_ENTITY_ATTRIBUTES_H_

#include <cstdint>
#include <string>

#include "posto/entity/guid.h"

namespace posto {

enum EntityType {
  ENTITY_PARTICIPANT = 1,
  ENTITY_WRITER = 2,
  ENTITY_READER = 3,
};

namespace entity {

struct Attributes {
  Guid guid;
  std::string host_name;
  uint64_t host_id;
  std::string participant_name;
  uint64_t participant_id;
  std::string topic;
};

inline const char* Type2Name(EntityType type) {
  switch (type) {
  case ENTITY_PARTICIPANT:
    return "PARTICIPANT";
  case ENTITY_WRITER:
    return "WRITER";
  case ENTITY_READER:
    return "READER";
  default:
    return "";
  }
}

}  // namespace entity
}  // namespace posto

#endif  // POSTO_ENTITY_ATTRIBUTES_H_
