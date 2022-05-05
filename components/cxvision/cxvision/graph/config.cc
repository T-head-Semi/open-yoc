/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <string.h>
#include <cstdint>
#include <utility>

#include <cJSON.h>

#include "cxvision/graph/config.h"

namespace cx {
namespace config {

constexpr char kDeviceId[] = "device_id";
constexpr char kProps[] = "props";
constexpr char kPlugin[] = "plugin";
constexpr char kNext[] = "next";
constexpr char kThread[] = "thread";
constexpr char kPriority[] = "priority";
constexpr char kStackSize[] = "stack_size";
constexpr char kDelimiter = '#';

static inline bool ParseNext(cJSON *obj, Next& out_next) {
  char* str = strchr(obj->valuestring, kDelimiter);
  if (str) {
    out_next.name = std::string(obj->valuestring,
        (uintptr_t)str - (uintptr_t)obj->valuestring);
    out_next.port = std::stoi(str + 1);
  } else {
    out_next.name = obj->valuestring;
    out_next.port = 0;
  }
  return true;
}

static inline bool ParseNode(cJSON *obj, Node& out_node) {
  cJSON *jattr = obj->child;
  while (jattr) {
    if (!strcmp(kProps, jattr->string) &&
        jattr->type == cJSON_Object) { /* props */
      cJSON *prop = jattr->child;
      while (prop) {
        if (prop->type == cJSON_String) {
          out_node.props[prop->string] = prop->valuestring;
        }
        prop = prop->next;
      }
    } else if (!strcmp(kDeviceId, jattr->string) &&
        jattr->type == cJSON_String) { /* device_id */
      out_node.device_id = jattr->valuestring;
    } else if (!strcmp(kPlugin, jattr->string) &&
        jattr->type == cJSON_String) { /* plugin */
      out_node.plugin = jattr->valuestring;
    } else if (!strcmp(kThread, jattr->string) &&
        jattr->type == cJSON_Object) { /* thread */
      cJSON *field = jattr->child;
      while (field) {
        if (!strcmp(kPriority, field->string) &&
            field->type == cJSON_String) { /* priority */
          out_node.thread_conf.priority = std::stoi(field->valuestring);
        } else if (!strcmp(kStackSize, field->string) &&
            field->type == cJSON_String) { /* stack_size */
          out_node.thread_conf.stack_size = std::stoi(field->valuestring);
        }
        field = field->next;
      }
      out_node.use_standalone_thread = true;
    } else if (!strcmp(kNext, jattr->string)) { /* next */
      if (jattr->type == cJSON_String) {
        Next next;
        if (ParseNext(jattr, next)) {
          out_node.next.emplace_back(std::move(next));
        }
      } else if (jattr->type == cJSON_Array) {
        cJSON *jnext = jattr->child;
        while (jnext) {
          if (jnext->type == cJSON_String) {
            Next next;
            if (ParseNext(jnext, next)) {
              out_node.next.emplace_back(std::move(next));
            }
          }
          jnext = jnext->next;
        }
      }
    }
    jattr = jattr->next;
  }
  out_node.name = obj->string;
  return true;
}

static inline bool ParsePipeline(cJSON *obj, Pipeline& out_pipeline) {
  cJSON *jnode = obj->child;
  while (jnode) {
    Node node;
    if (!ParseNode(jnode, node)) {
      return false;
    }
    out_pipeline.nodes.emplace_back(std::move(node));
    jnode = jnode->next;
  }
  out_pipeline.name = obj->string;
  return true;
}

static inline bool ParseRoot(cJSON *obj,
    std::vector<Pipeline>& out_pipelines) {
  cJSON *jpipeline = obj->child;
  while (jpipeline) {
    Pipeline pipeline;
    if (!ParsePipeline(jpipeline, pipeline)) {
      return false;
    }
    out_pipelines.emplace_back(std::move(pipeline));
    jpipeline = jpipeline->next;
  }
  return true;
}

bool LoadFromJson(const std::string& json,
                  std::vector<Pipeline>& out_pipelines) {
  bool ret = false;
  cJSON *root = cJSON_Parse(json.c_str());
  if (root) {
    ret = ParseRoot(root, out_pipelines);
    cJSON_Delete(root);
  }
  return ret;
}

void Dump(const std::vector<Pipeline>& pipelines) {
  printf("\nNumber pipelines: %zu\n", pipelines.size());
  for (const auto& pipeline : pipelines) {
    printf("%s\n", pipeline.name.c_str());
    for (const auto& node : pipeline.nodes) {
      printf("  %s\n", node.name.c_str());
      if (!node.device_id.empty())
        printf("    device_id: %s\n", node.device_id.c_str());
      if (!node.plugin.empty())
        printf("    plugin: %s\n", node.plugin.c_str());
      if (node.props.size() > 0) {
        printf("    props:\n");
        for (const auto& prop : node.props) {
          printf("      %s: %s\n", prop.first.c_str(), prop.second.c_str());
        }
      }
      if (node.use_standalone_thread) {
        printf("    use_standalone_thread: true\n");
        printf("      priority: %d\n", node.thread_conf.priority);
        printf("      stack_size: %d\n", node.thread_conf.stack_size);
      }
      if (node.next.size() > 0) {
        printf("    next: ");
        bool first = true;
        for (const auto& next : node.next) {
          if (first)
            first = false, printf("%s#%d", next.name.c_str(), next.port);
          else
            printf(", %s", next.name.c_str());
        }
        printf("\n");
      }
    }
  }
}

bool CreateGraph(const Pipeline& pipeline, Graph& out_graph) {
  auto& vertices = out_graph.vertices;
  auto& edges = out_graph.edges;
  for (const auto& node : pipeline.nodes) {
    Vertex v{&node, {}, {}};
    vertices.emplace_back(std::move(v));
  }

  int v_total = (int)vertices.size();
  int e_index = 0;
  for (int i = 0; i < v_total; i++) {
    for (const auto& next : vertices.at(i).node->next) {
      for (int j = 0; j < v_total; j++) {
        if (next.name == vertices.at(j).node->name) {
          Edge edge;
          edge.v_dst = j;
          edge.port_dst = next.port;
          edges.emplace_back(edge);
          vertices.at(i).e_out.emplace_back(e_index);
          vertices.at(j).e_in.emplace_back(e_index);
          ++e_index;
          break;
        }
      }
    }
  }
  out_graph.name = pipeline.name;
  return true;
}

void Dump(const Graph& graph) {
  auto& vertices = graph.vertices;
  auto& edges = graph.edges;

  printf("\nGraphs:\n");
  printf("%s\n", graph.name.c_str());
  for (const auto& v : vertices) {
    printf("  Plugin: %s\n", v.node->plugin.c_str());
    // {Replicas_ID}/graph_name/dst_node_name#dst_input_port
    for (int i : v.e_in) {
      std::string name = graph.name +
          "/" + vertices.at(edges.at(i).v_dst).node->name +
          "#" + std::to_string(edges.at(i).port_dst);
      printf("    Reader: %s\n", name.c_str());
    }
    for (int i : v.e_out) {
      std::string name = graph.name +
          "/" + vertices.at(edges.at(i).v_dst).node->name +
          "#" + std::to_string(edges.at(i).port_dst);
      printf("    Writer: %s\n", name.c_str());
    }
  }
}

}  // namespace config
}  // namespace cx
