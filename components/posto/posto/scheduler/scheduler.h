/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_SCHEDULER_SCHEDULER_H_
#define POSTO_SCHEDULER_SCHEDULER_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "posto/common/macros.h"
#include "posto/profile/statistics.h"

namespace posto {
namespace scheduler {

using TaskFunc = std::function<void(const std::shared_ptr<void>&)>;

class TaskImpl;
struct Task final {
  enum QueueFullPolicy {
    QFULL_POLICY_DROP,
    QFULL_POLICY_RING,
  };

  struct Attributes {
    constexpr static uint32_t kDefaultQueueCapacity = 64;

    uint32_t queue_capacity = kDefaultQueueCapacity;
    QueueFullPolicy queue_full_policy = QFULL_POLICY_DROP;
  };

  Task();
  bool Enqueue(const std::shared_ptr<void>& message);
  profile::Statistics* GetStatistics();

  std::unique_ptr<TaskImpl> impl;
};

class ExecutorImpl;
struct Executor final {
  struct Attributes {
    int priority;
    size_t stack_size;
  };

  Executor();
  std::unique_ptr<ExecutorImpl> impl;
};

class SchedulerImpl;
class Scheduler final {
 public:
  ~Scheduler();

  // Self-managed, executor will be stopped after all the tasks are removed.
  Executor* CreateExecutor(const Executor::Attributes* attr = nullptr);

  Task* CreateTask(TaskFunc&& func,
                   Executor* executor = nullptr, /* Bind to this executor */
                   const Task::Attributes* attr = nullptr);
  bool DestroyTask(Task* task);

  POSTO_DECLARE_SINGLETON(Scheduler)

 private:
  std::unique_ptr<SchedulerImpl> impl_;
};

}  // namespace scheduler
}  // namespace posto

#endif  // POSTO_SCHEDULER_SCHEDULER_H_
