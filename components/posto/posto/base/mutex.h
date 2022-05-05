/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_BASE_MUTEX_H_
#define POSTO_BASE_MUTEX_H_

#include <cstdint>

namespace posto {
namespace base {

class mutex {
public:
  mutex();
  ~mutex();

  void lock();
  void unlock();

private:
  mutex(const mutex&) = delete;

  uint32_t data_[16];
};

template <typename MutexT>
class lock_guard {
public:
  explicit lock_guard(MutexT& m) : m_(m) {
    m_.lock();
  }

  ~lock_guard() {
    m_.unlock();
  }

private:
  lock_guard(const lock_guard&) = delete;

  MutexT& m_;
};

} // namespace base
} // namespace posto

#endif  // POSTO_BASE_MUTEX_H_
