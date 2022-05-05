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
BOARDS_CONFIG_PATH=$BOARD_PATH/configs
BOOT0_BIN="${BOARD_PATH}/bootimgs/boot0"
BOOT_BIN="${BOARD_PATH}/bootimgs/boot"

cp ${BOOT0_BIN} $MK_GENERATED_PATH/data
cp ${BOOT_BIN} $MK_GENERATED_PATH/data
cp "$BOARDS_CONFIG_PATH/config.yaml" $MK_GENERATED_PATH/data/

CURDIR=${BASE_PWD}
HAASUI_SDK_DIR=$PATH_HAASUI_SDK
BOARD_DIR=$BOARD_PATH
OUT_DIR=${CURDIR}/out
RTOS_IMG=${CURDIR}/yoc.bin
FS_DATA_DIR=${CURDIR}/data
MK_GENERATED_IMGS_PATH=${CURDIR}/generated
CONFIG_YAML_FILE=$BOARDS_CONFIG_PATH/config.yaml

if [ ! -d $OUT_DIR ]; then
    mkdir $OUT_DIR
fi

if [ ! -d "${MK_GENERATED_IMGS_PATH}/data" ]; then
    echo "folder ${MK_GENERATED_IMGS_PATH}/data not find."
    exit 1
fi

$BOARD_DIR\\pack\\pack.exe -d 0 -r ${RTOS_IMG} -f ${FS_DATA_DIR} -o ${OUT_DIR} -m ${MK_GENERATED_IMGS_PATH} -c ${CONFIG_YAML_FILE}
