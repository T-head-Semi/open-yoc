## 概述

CxVision是ChiXiao Vision Platform的简称，支持插件开发模式和声明式流水线串接API。

## 组件安装
```bash
yoc init
yoc install cxvision
```

## 示例
### 插件开发
#### sample_plugin.h
```CPP
namespace sample {

class SamplePlugin final : public IPlugin {
public:
  bool Init(const std::map<std::string, std::string>& props) override;
  bool DeInit() override;
  bool Process(const std::vector<cx::BufferPtr>& bufferVec) override;
  bool Send(int port_id, const cx::BufferPtr& data);
[...]
};

}  // namespace sample
```

#### sample_plugin.cc
```CPP
namespace sample {

CX_REGISTER_PLUGIN(SamplePlugin);

bool SamplePlugin::Init(const std::map<std::string, std::string>& props) {
  auto iter = props.find("dataSource");
  if (iter != props.end()) {
    std::string dataSrouce = iter->second;
    [...]
  }
  [...]
}

bool SamplePlugin::Process(const std::vector<cx::BufferPtr>& bufferVec) {
  auto input = bufferVec.at(0);
  auto iMemory = input->GetMemory(0); // 0 is the index of memory
  auto iMeta = input->GetMetadata<cx::proto::SampleMeta>("input_meta");
  [...]
  cx::BufferPtr output = std::make_shared<cx::Buffer>();
  cx::MemoryPtr oMemory = cx::MemoryHelper::Malloc(1024);
  auto oMeta = std::make_shared<cx::proto::SampleMeta>();

  output->AddMemory(oMemory);
  output->SetMetadata("sample_meta", oMeta);

  [...]
  Send(0, output);
}

[...]

}  // namespace sample
```

### 流水线开发
```CPP
int main() {
  static const std::string json_str = R"({
    "pipeline_0": {
      "data_input": {
        "plugin": "DataInput",
        "next": "pre_process"
      },
      "pre_process": {
        "plugin": "PreProcess",
        "next": "inference"
      },
      "inference": {
        "device_id": "0",
        "plugin": "Inference",
        "props": {
          "model_path": "models/xxxx/yyyy"
        },
        "next": "post_process"
      },
      "post_process": {
        "plugin": "PostProcess"
      }
    }
  })";

  cx::GraphManager graphMgr(json_str);

  if (!graphMgr.Start()) {
    printf("Start graphs failed.\r\n");
  }

  while (1) {
    aos_msleep(2000);
  }

  return 0;
}
```

## 运行资源
无
