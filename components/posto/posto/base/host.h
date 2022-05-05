/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_BASE_HOST_H_
#define POSTO_BASE_HOST_H_

#include <cstdint>

namespace posto {
namespace base {
namespace host {

// Formated IPv4 address(4 bytes) for borad or CPU ID for AMP.
uint64_t get_id();
const char* get_name();

}  // namespace host
}  // namespace base
}  // namespace posto

#endif  // POSTO_BASE_HOST_H_
