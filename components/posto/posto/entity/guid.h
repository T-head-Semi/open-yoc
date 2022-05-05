/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_ENTITY_GUID_H_
#define POSTO_ENTITY_GUID_H_

#include <cstdint>
#include <cstring>

namespace posto {
namespace entity {

// GuidPrefix[12]
// + VendorId[2]
//   ProtocolVersion[2]
//   SequenceNumber[8]
//   + HostId[2]
//     ProcessId[2]
//     ParticipantId[4]
// EntityId[4]

struct Guid final {
  static constexpr unsigned int kSize = 16;
  unsigned char value[kSize]{};

  inline bool operator==(const Guid& other) const {
    return (0 == std::memcmp(value, other.value, kSize));
  }

  inline bool operator!=(const Guid& other) const {
    return (0 != std::memcmp(value, other.value, kSize));
  }

  inline bool operator<(const Guid& other) const {
    return (0 > std::memcmp(value, other.value, kSize));
  }
};

namespace guid {

void GeneratePrefix(Guid& guid, uint32_t participant_id);
void SetEntityId(Guid& guid, uint32_t entity_id);

}

}  // namespace entity
}  // namespace posto

#endif  // POSTO_ENTITY_GUID_H_
