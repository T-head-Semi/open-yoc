# 脚本说明

## pre_build.sh

在编译之前调用的脚本，用户可以根据需求添加功能。

## aft_build.sh

编译完成之后调用此脚本把elf文件转成bin文件，并且把config.yaml文件和其他的估计拷贝到指定位置通过product.exe工具进行打包生成镜像包。

## flash.init

烧录镜像的配置，可以配置一些寄存器，还有需要烧录哪些镜像。

## gdbinit

调试时用的脚本，跟cklink连接相关的配置。
