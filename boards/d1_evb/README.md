# the bsp for D1


# 注意

64位系统上要运行打包脚本需要先在终端里面执行以下命令开启相应的服务，否则会出现错误。

```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install libc6:i386
sudo apt-get install lib32ncurses5
sudo apt-get install lib32z1
sudo apt-get install lib32stdc++6

sudo apt install qemu-user-static
sudo update-binfmts --install i386 /usr/bin/qemu-i386-static --magic '\x7fELF\x01\x01\x01\x03\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x03\x00\x01\x00\x00\x00' --mask '\xff\xff\xff\xff\xff\xff\xff\xfc\xff\xff\xff\xff\xff\xff\xff\xff\xf8\xff\xff\xff\xff\xff\xff\xff'
sudo service binfmt-support start
```
