
NAME := breeze

$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 1.0.0
$(NAME)_SUMMARY := breeze provides secure BLE connection to Alibaba IoT cloud and services.

$(NAME)_SOURCES += core/core.c
$(NAME)_SOURCES += core/transport.c
$(NAME)_SOURCES += core/ble_service.c
$(NAME)_SOURCES += core/sha256.c
$(NAME)_SOURCES += core/utils.c
#$(NAME)_SOURCES += core/extcmd.c

GLOBAL_INCLUDES += include hal/include api

$(NAME)_COMPONENTS := chip_code

btstack ?= zephyr
ifeq (zephyr, $(btstack))
$(NAME)_COMPONENTS += framework.bluetooth.breeze.hal.ble
endif

$(NAME)_SOURCES += api/breeze_export.c

bz_en_auth ?= 1
ifeq ($(bz_en_auth), 1)
GLOBAL_DEFINES += EN_AUTH
$(NAME)_SOURCES += core/auth.c
ifeq ($(offline_auth), 1)
GLOBAL_DEFINES += EN_AUTH_OFFLINE
endif
endif

bz_en_awss ?= 0
ifeq ($(bz_en_awss), 1)
GLOBAL_DEFINES += EN_COMBO_NET
$(NAME)_SOURCES += core/extcmd.c
$(NAME)_SOURCES += api/breeze_awss_export.c
endif

bz_long_mtu ?= 1
ifeq ($(bz_long_mtu), 1)
GLOBAL_DEFINES += EN_LONG_MTU
endif

