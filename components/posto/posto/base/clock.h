/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_BASE_CLOCK_H_
#define POSTO_BASE_CLOCK_H_

#include <cstdint>

namespace posto {
namespace base {
namespace clock {

uint32_t now_ms();
uint64_t now_ns();

}  // namespace clock
}  // namespace base
}  // namespace posto 

#endif  // POSTO_BASE_CLOCK_H_
