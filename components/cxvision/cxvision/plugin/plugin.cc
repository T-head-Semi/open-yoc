/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <utility>

#include "cxvision/plugin/plugin.h"

namespace cx {

bool PluginBase::Send(int port, const cx::BufferPtr& data) {
  auto writer = writers_.at(port);
  if (writer) {
    writer->Write(data);
  }
  return true;
}

PluginBase::~PluginBase() {}

bool PluginBase::_CreateIoPorts(const cx::config::Graph& graph,
                                const cx::config::Vertex& v) {
  auto e_in_size = v.e_in.size();
  dataMatrix_.resize(e_in_size);
  for (size_t i = 0; i < e_in_size; ++i) {
    dataMatrix_[i].resize(e_in_size);
  }

  auto& vertices = graph.vertices;
  auto& edges = graph.edges;

  participant_ = posto::Domain::CreateParticipant("any");
  posto::scheduler::Executor* executor = nullptr;
  if (v.node->use_standalone_thread) {
    posto::scheduler::Executor::Attributes attr;
    attr.priority = v.node->thread_conf.priority;
    attr.stack_size = (size_t)v.node->thread_conf.stack_size;
    executor = posto::scheduler::Scheduler::Instance()->CreateExecutor(&attr);
  }

  // {Replicas_ID}/graph_name/dst_node_name#dst_input_port
  for (int i : v.e_in) {
    int port = edges.at(i).port_dst;
    std::string name = graph.name +
        "/" + vertices.at(edges.at(i).v_dst).node->name +
        "#" + std::to_string(port);
    auto reader = participant_->CreateReader<cx::Buffer>(name,
        [this, port](const cx::BufferPtr& data) {
      dataMatrix_[port][port] = data;
      if (running_) {
        this->Process(dataMatrix_.at(port));
      }
    }, executor);
    readers_.push_back(std::move(reader));
  }

  for (int i : v.e_out) {
    std::string name = graph.name +
        "/" + vertices.at(edges.at(i).v_dst).node->name +
        "#" + std::to_string(edges.at(i).port_dst);
    auto writer = participant_->CreateWriter<cx::Buffer>(name);
    writers_.push_back(std::move(writer));
  }
  return true;
}

void PluginBase::_Start() { running_ = true; }

void PluginBase::_PrintStatistics(int level) {
  for (const auto& reader : readers_) {
    TAB_PRINT(level, "input: %s\r\n", reader->attributes().topic.c_str());
    auto provider = reader->GetStatistics();
    if (provider)
      provider->Print(level + 1);
  }

  for (const auto& writer : writers_) {
    TAB_PRINT(level, "output: %s\r\n", writer->attributes().topic.c_str());
    auto provider = writer->GetStatistics();
    if (provider)
      provider->Print(level + 1);
  }
}

namespace plugin {

Manager::Manager() {}

bool Manager::Register(const std::string& name, IRegistrar* registrar) {
  posto::base::lock_guard<posto::base::mutex> lock(mutex_);
  const auto& it = plugins_.find(name);
  if (it != plugins_.end())
    return false;

  plugins_[name] = registrar;
  return true;
}

PluginBase* Manager::Create(const std::string& name) {
  posto::base::lock_guard<posto::base::mutex> lock(mutex_);
  const auto& it = plugins_.find(name);
  if (it != plugins_.end()) {
    auto instance = it->second->New();
    instances_.emplace_back(instance);
    return instance;
  }

  return nullptr;
}

void Manager::Destroy(PluginBase* plugin) {
  // TODO
}

}  // namespace plugin
}  // namespace cx
