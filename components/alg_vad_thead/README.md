## 概述

Thead算法的VAD处理节点，基于Thead的语音算法应用框架，主要功能为在关键词唤醒后检测信号处理后的语音数据是否有声音来给断句做判断依据。

## 组件安装
```bash
yoc init
yoc install alg_vad_thead
```
## 接口
```bash
    // 输入
    // ssp处理后的pcm数据信息
    auto iMeta0 = ptr0->GetMetadata<SspOutMessageT>("ssp_param");
    // 唤醒信息
    auto iMeta1 = ptr1->GetMetadata<InferOutMessageT>("kws_param");


    // 输出
    // 发送断句信息
    auto oMeta = std::make_shared<VadOutMessageT>();
    oMeta->body().set_vad_status(0);

    auto output = std::make_shared<CxpiBuffer>();
    output->SetMetadata("vad_param", oMeta);

    Send(0, output);
```

## 示例
无

## 运行资源
无