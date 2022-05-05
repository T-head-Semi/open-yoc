/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_POSTO_H_
#define POSTO_POSTO_H_

#include "posto/base/clock.h"
#include "posto/domain/domain.h"
#include "posto/message/message.h"
#include "posto/participant/participant.h"
#include "posto/participant/reader.h"
#include "posto/participant/writer.h"
#include "posto/profile/statistics.h"
#include "posto/transport/ioblock/ioblock.h"

namespace posto {

bool Init();

}  // namespace posto

#endif  // POSTO_POSTO_H_
