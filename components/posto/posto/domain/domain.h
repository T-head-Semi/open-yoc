/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_DOMAIN_DOMAIN_H_
#define POSTO_DOMAIN_DOMAIN_H_

#include <memory>
#include <string>

#include "posto/participant/participant.h"

namespace posto {

class Domain final {
public:
  static std::shared_ptr<Participant> CreateParticipant(const std::string&);
};

}  // namespace posto

#endif  // POSTO_DOMAIN_DOMAIN_H_
