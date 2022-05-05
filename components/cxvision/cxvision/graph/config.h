/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef CXVISION_GRAPH_CONFIG_H_
#define CXVISION_GRAPH_CONFIG_H_

#include <map>
#include <string>
#include <vector>

namespace cx {
namespace config {

struct Next {
  std::string name;
  int port;
};

struct Thread {
  int priority;
  int stack_size;
};

struct Node {
  std::string name;
  std::string device_id;
  std::string plugin;
  std::map<std::string, std::string> props;
  std::vector<Next> next;
  bool use_standalone_thread = false;
  Thread thread_conf;
};

struct Pipeline {
  std::string name;
  std::vector<Node> nodes;
};

struct Edge {
  int v_dst;
  int port_dst;
};

struct Vertex {
  const Node* node;
  std::vector<int> e_in;
  std::vector<int> e_out;
};

struct Graph {
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<Edge> edges;
};

bool LoadFromJson(const std::string& json,
                  std::vector<Pipeline>& out_pipelines);
void Dump(const std::vector<Pipeline>& pipelines);
bool CreateGraph(const Pipeline& pipeline, Graph& out_graph);
void Dump(const Graph& graph);

}  // namespace config
}  // namespace cx

#endif  // CXVISION_GRAPH_CONFIG_H_
