#!/bin/bash

CURDIR=`pwd`
BOARD_DIR=../../boards/d1_evb
OUT_DIR="${CURDIR}/out"
RTOS_IMG="${CURDIR}/yoc.bin"
FS_DATA_DIR="${CURDIR}/data"
MK_GENERATED_IMGS_PATH=${CURDIR}/generated
CONFIG_YAML_FILE=${BOARD_DIR}/configs/config.yaml

if [ ! -d "${MK_GENERATED_IMGS_PATH}/data" ]; then
    echo "folder ${MK_GENERATED_IMGS_PATH}/data not find."
    exit 1
fi

# # copy wifi firmware
# echo "update ${FS_DATA_DIR}/mnt/D/data/firmware"
# CHIP_D1_DIR=../../components/chip_d1
# mkdir -p ${FS_DATA_DIR}/mnt/D/data/firmware
# cp ${CHIP_D1_DIR}/firmware/* ${FS_DATA_DIR}/mnt/D/data/firmware/ -arf

# copy haasui resources
echo "update ${FS_DATA_DIR}/resources"
HAASUI_SDK_DIR=../../components/haasui_sdk
mkdir -p ${CURDIR}/.data/resources
cp ${HAASUI_SDK_DIR}/resources/* ${CURDIR}/.data/resources -arf
if [ -d "${FS_DATA_DIR}/resources" ]; then
    cp ${FS_DATA_DIR}/resources/* ${CURDIR}/.data/resources -arf
    rm -rf ${FS_DATA_DIR}/resources
else
    mkdir -p ${FS_DATA_DIR}/resources
fi
cp ${CURDIR}/.data/resources ${FS_DATA_DIR}/resources -arf
rm -rf ${CURDIR}/.data

python ${BOARD_DIR}/pack/pack.py -d 0 -r ${RTOS_IMG} -f ${FS_DATA_DIR} -o ${OUT_DIR} -m ${MK_GENERATED_IMGS_PATH} -c ${CONFIG_YAML_FILE}