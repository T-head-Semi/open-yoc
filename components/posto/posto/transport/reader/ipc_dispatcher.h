/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_READER_IPC_DISPATCHER_H_
#define POSTO_TRANSPORT_READER_IPC_DISPATCHER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <map>

#include "posto/base/mutex.h"
#include "posto/common/macros.h"
#include "posto/entity/attributes.h"
#include "posto/transport/amp/amp_ipc.h"

namespace posto {
namespace transport {

class IpcDispatcher;
using IpcDispatcherPtr = IpcDispatcher*;
using IpcListener = std::function<void(uint8_t* data, size_t size)>;

class IpcDispatcher {
 public:
  ~IpcDispatcher();

  void AddListener(const entity::Attributes&, const IpcListener&);
  void RemoveListener(const entity::Attributes&);

 private:
  void OnData(uint8_t* data, size_t size);

  AmpIpc* amp_ipc_;
  base::mutex mutex_;
  std::map<entity::Guid, IpcListener> listeners_;

  POSTO_DECLARE_SINGLETON(IpcDispatcher)
};

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_READER_IPC_DISPATCHER_H_
