/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef CXVISION_PLUGIN_PLUGIN_H_
#define CXVISION_PLUGIN_PLUGIN_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <posto/base/mutex.h>
#include <posto/common/macros.h>
#include <posto/posto.h>

#include "cxvision/graph/config.h"
#include "cxvision/plugin/buffer.h"

#define CX_REGISTER_PLUGIN(Class) \
    static ::cx::plugin::Registrar<Class> registrar_##Class(#Class)

namespace cx {

class GraphManager;

class PluginBase {
public:
  virtual ~PluginBase();

  virtual bool Init(const std::map<std::string, std::string>& props) = 0;
  virtual bool DeInit() = 0;
  virtual bool Process(const std::vector<cx::BufferPtr>& data_vec) = 0;

  bool Send(int port, const cx::BufferPtr& data);

private:
  bool _CreateIoPorts(const cx::config::Graph& graph,
                      const cx::config::Vertex& v);
  void _Start();
  void _PrintStatistics(int level);

private:
  std::atomic<bool> running_{false};
  std::shared_ptr<posto::Participant> participant_;
  std::vector<std::shared_ptr<posto::Writer<cx::Buffer>>> writers_;
  std::vector<std::shared_ptr<posto::Reader<cx::Buffer>>> readers_;

  // Lock free
  std::vector<std::vector<cx::BufferPtr>> dataMatrix_;

friend class GraphManager;
};

namespace plugin {

class IRegistrar {
 public:
  virtual ~IRegistrar() = default;

  virtual PluginBase* New() = 0;
};

class Manager final {
 public:
  bool Register(const std::string& name, IRegistrar* registrar);

  PluginBase* Create(const std::string& name);
  void Destroy(PluginBase* plugin);

 private:
  posto::base::mutex mutex_;
  std::map<std::string, IRegistrar*> plugins_;
  std::vector<PluginBase*> instances_;

  POSTO_DECLARE_SINGLETON(Manager)
};

template <typename Class>
class Registrar final : public IRegistrar {
public:
  explicit Registrar(const std::string& name) {
    Manager::Instance()->Register(name, this);
  }

  PluginBase *New() override {
    return new Class();
  }
};

}  // namespace plugin
}  // namespace cx

#endif  // CXVISION_PLUGIN_PLUGIN_H_
