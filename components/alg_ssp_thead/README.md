## 概述

Thead算法的SSP前处理节点，基于Thead的语音算法应用框架，主要功能为原始MIC语音数据进行降噪、自适应增益及回声消除。

## 组件安装
```bash
yoc init
yoc install alg_ssp_thead
```
## 接口
```bash
    // 输入
    // mic原始pcm数据
  auto iMemory = data_vec.at(0)->GetMemory(0);
  int16_t *data_in = (int16_t *)iMemory->data();

  auto iMeta = data_vec.at(0)->GetMetadata<DataInputMessageT>("alsa_param");

    // 输出
    // ssp处理后的pcm数据
  auto oMeta = std::make_shared<SspOutMessageT>();
  oMeta->body().set_chn_num(DATA_OUT_CHAN);
  oMeta->body().set_vad_res(ret);
  oMeta->body().set_sample_rate(iMeta->body().sample_rate());
  oMeta->body().set_frame(iMeta->body().frame());

  auto output = std::make_shared<CxpiBuffer>();
  output->AddMemory(oMemory);
  output->SetMetadata("ssp_param", oMeta);

  Send(0, output);     // inference
```

## 示例
无

## 运行资源
无