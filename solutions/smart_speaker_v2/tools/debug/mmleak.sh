#!/bin/sh

TOOL_PATH=`which riscv64-unknown-elf-gcc`

if [ -z $1 ]; then
	echo "Usage: mmleak test.log"
	echo "    elf filename: yoc.elf"
	exit
fi

python2 coredump_parser_mmleak.py $1 yoc.elf -p ${TOOL_PATH} > $1.mm.txt
