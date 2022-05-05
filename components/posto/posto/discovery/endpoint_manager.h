/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_DISCOVERY_ENDPOINT_MANAGER_H_
#define POSTO_DISCOVERY_ENDPOINT_MANAGER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "posto/base/mutex.h"
#include "posto/common/macros.h"
#include "posto/entity/attributes.h"

namespace posto {
namespace discovery {

class EndpointManager;
using EndpointManagerPtr = EndpointManager*;

class EndpointManager {
 public:
  using EntityAttrVec = std::vector<entity::Attributes*>;
  using EntityAttrPtr = std::shared_ptr<entity::Attributes>;

  ~EndpointManager();

  bool Join(const entity::Attributes& attr, EntityType type);
  bool Leave(const entity::Attributes& attr, EntityType type);
  void GetReadersByTopic(const std::string& topic, EntityAttrVec* readers);

 private:
  bool Init();

  base::mutex mutex_;
  std::list<EntityAttrPtr> writers_;
  std::list<EntityAttrPtr> readers_;

  POSTO_DECLARE_SINGLETON(EndpointManager)
};

}  // namespace discovery
}  // namespace posto

#endif  // POSTO_DISCOVERY_ENDPOINT_MANAGER_H_
