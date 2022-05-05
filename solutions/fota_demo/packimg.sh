#!/bin/bash

CURDIR=`pwd`
BOARD_DIR=../../boards/d1_evb
OUT_DIR="${CURDIR}/out"
RTOS_IMG="${CURDIR}/yoc.bin"
FS_DATA_DIR="${CURDIR}/data"
MK_GENERATED_IMGS_PATH=${CURDIR}/generated
CONFIG_YAML_FILE=${CURDIR}/configs/config.yaml
FACTORY_ZIP=${CURDIR}/$1

if [ ! -d "${MK_GENERATED_IMGS_PATH}/data" ]; then
    echo "folder ${MK_GENERATED_IMGS_PATH}/data not find."
    exit 1
fi

# # copy wifi firmware
# echo "update ${FS_DATA_DIR}/mnt/D/data/firmware"
# CHIP_D1_DIR=../../components/chip_d1
# mkdir -p ${FS_DATA_DIR}/mnt/D/data/firmware
# cp ${CHIP_D1_DIR}/firmware/* ${FS_DATA_DIR}/mnt/D/data/firmware/ -arf

python ${BOARD_DIR}/pack/pack.py -d 0 -r ${RTOS_IMG} -f ${FS_DATA_DIR} -o ${OUT_DIR} -m ${MK_GENERATED_IMGS_PATH} -c ${CONFIG_YAML_FILE} -z ${FACTORY_ZIP}