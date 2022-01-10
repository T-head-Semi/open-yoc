#!/bin/bash

if [ -d node_modules/falcon-ui ]; then
    rm -rf node_modules/falcon-ui
    ln -sv ../../iot-falcon-ui node_modules/falcon-ui
fi

aiot-cli-g.sh preview
