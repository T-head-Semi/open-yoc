/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_AMPIPC_H_
#define POSTO_TRANSPORT_AMPIPC_H_

#include <cstdint>
#include <functional>
#include <map>

#include "posto/base/mutex.h"
#include "posto/common/macros.h"

namespace posto {

using IpcMessageHandler =
    std::function<void(uint8_t*, size_t, uint32_t)>;

// A transport driver based on AMP IPC for send/recv messages.
class AmpIpc final {
public:
  enum SpecialPort : uint16_t {
    SD_PORT = 1,   // Service discovery
    SHM_REF_PORT,  // Shared memory ref/unref
    USER_MSG_PORT, // User message
  };

  bool Init();

  bool Bind(uint16_t port, const IpcMessageHandler& msg_handler);
  void Unbind(uint16_t port);

  int SendTo(uint8_t* data, size_t size, uint32_t cpu_id, uint16_t port);
  void Broadcast(uint8_t* data, size_t size, uint16_t port);

private:
  base::mutex mutex_;
  std::map<uint16_t, IpcMessageHandler> listeners_;

  IpcMessageHandler sd_handler_;
  IpcMessageHandler shm_ref_handler_;
  IpcMessageHandler user_msg_handler_;

  POSTO_DECLARE_SINGLETON(AmpIpc)
};

}  // namespace posto

#endif  // POSTO_TRANSPORT_AMPIPC_H_
