#!/bin/bash

if [ "${CDK_VERSION}" != '' ];then
#Parse from yaml
BOARD_NAME=`grep "board_name" package.yaml | awk '{printf "%s", toupper($2)}'| xargs echo `

BOARD_PATH=PATH_${BOARD_NAME}
BOARD_PATH=${!BOARD_PATH}

echo CDK_Version: ${CDK_VERSION}
echo Board Path: ${BOARD_PATH}

if [ ! -d  ${BOARD_PATH} ];then
echo "Boards missing!!!!!!!"
fi

cp ${BOARD_PATH}/script/pre_build.sh cdk/script/
cp ${BOARD_PATH}/script/aft_build.sh cdk/script/
cdk/script/pre_build.sh

fi
