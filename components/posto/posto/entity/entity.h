/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_ENTITY_ENTITY_H_
#define POSTO_ENTITY_ENTITY_H_

#include "posto/entity/attributes.h"

namespace posto {
namespace entity {

class Entity {
public:
  virtual ~Entity() {};

  inline const Guid& guid() const { return attr_.guid; }
  inline Guid& guid() { return attr_.guid; }
  inline const Attributes& attributes() const { return attr_; }
  inline Attributes& attributes() { return attr_; }

protected:
  Attributes attr_{};
};

}  // namespace entity
}  // namespace posto

#endif  // POSTO_ENTITY_ENTITY_H_
