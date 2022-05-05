/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_COMMON_MACROS_H_
#define POSTO_COMMON_MACROS_H_

#include <atomic>
#include <new>
#include "posto/base/mutex.h"

#define POSTO_DISALLOW_COPY_AND_ASSIGN(Class) \
  Class(const Class&) = delete; \
  Class& operator=(const Class&) = delete;

#define POSTO_DECLARE_SINGLETON(Class)                                 \
 public:                                                               \
  static Class *Instance() {                                           \
    static posto::base::mutex mtx;                                     \
    static std::atomic<Class*> instance;                               \
    if (!instance) {                                                   \
      mtx.lock();                                                      \
      if (!instance) {                                                 \
        instance = new (std::nothrow) Class();                         \
      }                                                                \
      mtx.unlock();                                                    \
    }                                                                  \
    return instance;                                                   \
  }                                                                    \
                                                                       \
 private:                                                              \
  Class();                                                             \
  POSTO_DISALLOW_COPY_AND_ASSIGN(Class)

#endif  // POSTO_COMMON_MACROS_H_
