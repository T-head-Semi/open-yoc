/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <assert.h>

#include "cxvision/plugin/buffer.h"

namespace cx {

size_t Buffer::_ByteSizeLong() const {
  // TODO
  assert(false && "Unimplemented");
  return 0;
}

bool Buffer::_SerializeToArray(void* data, size_t size) const {
  // TODO
  assert(false && "Unimplemented");
  return false;
}

bool Buffer::_ParseFromArray(const void* data, size_t size) {
  // TODO
  assert(false && "Unimplemented");
  return false;
}

}  // namespace cx
