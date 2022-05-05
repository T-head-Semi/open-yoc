# 如何使用protoc 生成代码


## 生成命令
```
./protoc --cpp_out=. xxx.proto
```
执行命令后会在当前目录下 xxx.pb.cc xxx.pb.h

## proto文件规则

[文件规则](https://developers.google.com/protocol-buffers/docs/reference/proto3-spec)