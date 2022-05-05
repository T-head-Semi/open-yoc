#!/bin/sh

#flash
if [ "$1" = "flash" ];then
     product flash ./generated/images.zip -w prim -f ./script/w800_flash.elf
elif [ "$1" = "flashall" ];then
	 product flash ./generated/images.zip -a -f ./script/w800_flash.elf
else
  echo "err param"
fi