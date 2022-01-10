#!/bin/sh
BASE_PWD=`pwd`
MK_GENERATED_PATH=generated

echo "[INFO] Generated output files ..."

rm -fr $MK_GENERATED_PATH
mkdir -p $MK_GENERATED_PATH/data/

OBJCOPY=riscv64-unknown-elf-objcopy

#output yoc.bin
ELF_NAME=`ls Obj/*.elf`
$OBJCOPY -O binary $ELF_NAME yoc.bin

#Prepare bin
BOARDS_CONFIG_PATH="../../boards/d1_evb/configs"
BOARD_PATH="../../boards/d1_evb"
BOOT0_BIN="${BOARD_PATH}/bootimgs/boot0"
BOOT_BIN="${BOARD_PATH}/bootimgs/boot"

cp ${BOOT0_BIN} $MK_GENERATED_PATH/data
cp ${BOOT_BIN} $MK_GENERATED_PATH/data
cp "$BOARDS_CONFIG_PATH/config.yaml" $MK_GENERATED_PATH/data/

cmd /c packimg.bat
