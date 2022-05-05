/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_TRANSPORT_READER_READER_H_
#define POSTO_TRANSPORT_READER_READER_H_

#include <functional>
#include <memory>

#include "posto/transport/reader/intra_reader.h"
#include "posto/transport/reader/ipc_reader.h"

namespace posto {
namespace transport {

template <typename MessageT>
class Reader {
 public:
  using MessagePtr = std::shared_ptr<MessageT>;
  using MessageCallback = std::function<void(const MessagePtr&,
      const entity::Attributes&)>;
  using IntraReaderPtr = std::shared_ptr<IntraReader<MessageT>>;
  using IpcReaderPtr = std::shared_ptr<IpcReader<MessageT>>;

  Reader(const entity::Attributes& attr, const MessageCallback& callback);
  ~Reader();

 private:
  void Init(const MessageCallback& callback);

  entity::Attributes attr_;
  IntraReaderPtr intra_reader_;
  IpcReaderPtr ipc_reader_;
};

template <typename MessageT>
Reader<MessageT>::Reader(
    const entity::Attributes& attr,
    const MessageCallback& callback) : attr_(attr) {
  Init(callback);
}

template <typename MessageT>
Reader<MessageT>::~Reader() {}

template <typename MessageT>
void Reader<MessageT>::Init(const MessageCallback& callback) {
  intra_reader_ =
      std::make_shared<IntraReader<MessageT>>(attr_, callback);
  ipc_reader_ =
      std::make_shared<IpcReader<MessageT>>(attr_, callback);
}

}  // namespace transport
}  // namespace posto

#endif  // POSTO_TRANSPORT_READER_READER_H_
