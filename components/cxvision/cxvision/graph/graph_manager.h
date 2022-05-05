/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef CXVISION_GRAPH_MANAGER_H_
#define CXVISION_GRAPH_MANAGER_H_

#include <map>
#include <string>

#include "cxvision/plugin/plugin.h"

namespace cx {

class GraphManager final {
 public:
  explicit GraphManager(const std::string& jstr);
  ~GraphManager();

  bool Start();
  bool Stop();

  void PrintStatistics();

 private:
  struct VertexInfo {
    std::string name;
    PluginBase* plugin;
  };

  std::string json_;
  std::map<std::string, std::vector<VertexInfo>> graphs_;
};

}  // namespace cx

#endif  // CXVISION_GRAPH_MANAGER_H_
