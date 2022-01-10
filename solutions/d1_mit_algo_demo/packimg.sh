#!/bin/bash

CURDIR=`pwd`
PACK_DIR=../../boards/d1_evb/pack
OUT_DIR="${CURDIR}/out"
RTOS_IMG="${CURDIR}/yoc.bin"
FS_IMG="${CURDIR}/littlefs.bin"
MK_GENERATED_IMGS_PATH=${CURDIR}/generated
SYS_PARTITION_FILE=${CURDIR}/configs/sys_partition_nor.fex

if [ ! -f ${RTOS_IMG} ]; then
    echo "file ${RTOS_IMG} not find"
    exit 1
else
    if [ ! -d "${MK_GENERATED_IMGS_PATH}/data" ]; then
        echo "folder ${MK_GENERATED_IMGS_PATH}/data not find."
        exit 1
    fi
    # echo "the rtos image is ${RTOS_IMG}"
    bash ${PACK_DIR}/pack.sh ${RTOS_IMG} ${FS_IMG} ${OUT_DIR} ${MK_GENERATED_IMGS_PATH} ${SYS_PARTITION_FILE}
fi
