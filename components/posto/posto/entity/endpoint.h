/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_ENTITY_ENDPOINT_H_
#define POSTO_ENTITY_ENDPOINT_H_

#include "posto/entity/entity.h"

namespace posto {
namespace entity {

class Endpoint : public Entity {
public:
  virtual ~Endpoint() {}
};

}  // namespace entity
}  // namespace posto

#endif  // POSTO_ENTITY_ENDPOINT_H_
