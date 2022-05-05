## 概述
Thead算法KWS推理应用节点，基于Thead的语音算法应用框架，主要功能为关键词唤


## 组件安装
```bash
yoc init
yoc install alg_kws_thead
```
## 接口
```bash
    // 输入
    // ssp处理后的pcm数据
    auto iMemory = data_vec.at(0)->GetMemory(0);
    int16_t *data_in = (int16_t *)iMemory->data();
    auto iMeta = data_vec.at(0)->GetMetadata<ssp_out_st>("ssp_param");

    // 输出
    // 发送唤醒信息
    auto oMeta = std::make_shared<InferOutMessageT>();
    oMeta->body().set_kws_chn(kws_chn_id_);
    oMeta->body().set_kws_id(output_data_[kws_chn_id_][0]);
    oMeta->body().set_kws_score(max_score_);
    oMeta->body().set_first_wakeup(first_wkup);

    auto output = std::make_shared<CxpiBuffer>();
    output->SetMetadata("kws_param", oMeta);

    Send(0, output);
    Send(1, output);
```

## 示例
无

## 运行资源
无