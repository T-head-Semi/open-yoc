#ifndef CXVISION_CXVISION_H_
#define CXVISION_CXVISION_H_

#include "cxvision/graph/graph_manager.h"
#include "cxvision/plugin/plugin.h"
#ifdef CXVISION_USE_PROTOBUF
#include "cxvision/proto/simple_meta.pb.h"
#include "cxvision/proto/voice.pb.h"
#else
#include "cxvision/proto/simple_meta.h"
#include "cxvision/proto/voice.h"
#endif

#endif  // CXVISION_CXVISION_H_
