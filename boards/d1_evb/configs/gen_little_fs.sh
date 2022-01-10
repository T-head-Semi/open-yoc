#!/usr/bin/env bash
SCRIPT_ABS_DIR=$(dirname `readlink -f $0`)

if [ "$(uname)" = "Darwin" ]; then
	${SCRIPT_ABS_DIR}/mklittlefs_osx -c ./data -d 5 -b 4096 -p 1792 -s 4530176 ./littlefs.bin
elif [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
	${SCRIPT_ABS_DIR}/mklittlefs_linux -c ./data -d 5 -b 4096 -p 1792 -s 4530176 ./littlefs.bin >/dev/null
elif [ "$(expr substr $(uname -s) 1 10)" = "MINGW32_NT" ]; then
	echo
fi

