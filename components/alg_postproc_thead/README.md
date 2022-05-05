## 概述

Thead算法的后处理节点，基于Thead的语音算法应用框架，主要功能为上云数据的缓冲，唤醒状态的更新处理，断句处理及超时断句。

## 组件安装
```bash
yoc init
yoc install alg_postproc_thead
```
## 接口
```bash
    // 输入
    // ssp处理后的pcm数据
    auto iMemory0 = ptr0->GetMemory(0);
    auto iMeta0 = ptr0->GetMetadata<SspOutMessageT>("ssp_param");

    // 唤醒消息
    auto iMeta1 = ptr1->GetMetadata<InferOutMessageT>("kws_param");
    kws_chn = iMeta1->body().kws_chn();

    // vad消息
    auto iMeta2 = ptr2->GetMetadata<VadOutMessageT>("vad_param");

    // 输出
    // 发布唤醒开始 / 结束消息
    writer_->Write(msg);
```
## 接口详细说明
```
### voice_get_pcm_data
` int voice_get_pcm_data(void *data, int len);`

- 功能描述:
   - 获取经过ssp处理后的pcm数据

- 参数:
   - `data`: 获取ssp处理后的pcm数据的buffer。
   - `len`：获取pcm数据的最大长度。

- 返回值:
   - 实际获取pcm数据的长度。
```
## 示例
无

## 运行资源
无