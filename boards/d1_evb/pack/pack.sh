#!/bin/bash

CURDIR=$(dirname `readlink -f $0`)
RTOS_IMG=$1
FS_IMG=$2
OUT_DIR=$3
MK_GENERATED_IMGS_PATH=$4
SYS_PARTITION_FILE=$5
# echo "${CURDIR}"
# echo "${RTOS_IMG} ${FS_IMG} ${OUT_DIR}"
MELIS_BASE=${CURDIR}
# if ! which product >/dev/null 2>&1; then
#     export PATH=${PATH}:${CURDIR}/../configs
# fi
# if ! product version >/dev/null 2>&1; then
#     echo "can't find product."
#     exit
# fi
PRODUCT=${CURDIR}/../configs/product

if [ ! "${SYS_PARTITION_FILE}" ] || [ ! -f ${SYS_PARTITION_FILE} ]; then
    SYS_PARTITION_FILE=${CURDIR}/../configs/sys_partition_nor.fex
fi

if [ ! -f ${SYS_PARTITION_FILE} ]; then
    echo "the sys partition file is not exist."
    exit
fi
python ${CURDIR}/tools/packtool/pack.py 0 ${RTOS_IMG} ${FS_IMG} ${OUT_DIR} ${MK_GENERATED_IMGS_PATH} ${SYS_PARTITION_FILE} ${PRODUCT}
