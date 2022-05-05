## 概述

Posto是一个分布式通信框架，采用Pub/Sub通信模式。
概念上遵循OMG RTPS规范，并且为异构多核做了适配，可以在YoC AMP环境使用。

## 组件安装
```bash
yoc init
yoc install posto
```

## 示例
### Writer侧(cpu2)
```CPP
#include <posto/posto.h>
#include "mytest.pb.h"

using MyTestMessageT = posto::Message<posto::proto::MyTest>;

static void _test_entry(void *arg) {
  posto::Init();

  auto participant_ = posto::Domain::CreateParticipant("data_producer");
  auto writer_ = participant_->CreateWriter<MyTestMessageT>("posto/mytest");

  while (1) {
    auto msg = std::make_shared<MyTestMessageT>();

    msg->body().set_version("3.17.3");
    [...]
    writer_->Write(msg);

    aos_msleep(1000);
  }

  aos_task_exit(0);
}

void posto_test(void) {
  static aos_task_t _task;
  aos_task_new_ext(&_task, "posto_test", _test_entry, nullptr, 1024 * 8, 9);
}
```

### Reader侧(cpu0)
```CPP
#include <posto/posto.h>
#include "mytest.pb.h"

using MyTestMessageT = posto::Message<posto::proto::MyTest>;

static void _test_entry(void *arg) {
  posto::Init();

  auto participant_ = posto::Domain::CreateParticipant("data_consumer");
  auto reader_ = participant_->CreateReader<MyTestMessageT>("posto/mytest",
      [] (const std::shared_ptr<MyTestMessageT>& msg) {
    printf("[CPU0]Message got, version: %s\n", msg->body().version().c_str());
    [...]
  });

  while (1) {
    aos_msleep(2000);
  }

  aos_task_exit(0);
}

void posto_test(void) {
  static aos_task_t _task;
  aos_task_new_ext(&_task, "posto_test", _test_entry, nullptr, 1024 * 8, 9);
}
```

## 运行资源
当前实现配置了2个线程，每线程8K stack的调度线程池。
