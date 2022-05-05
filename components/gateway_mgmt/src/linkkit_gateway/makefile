
include linkkit_gateway.mk
target = linkkit_gateway

CFLAGS += -DALCS_ENABLED

INCLUDE_PATH = -I$(PWD)
INCLUDE      = $(INCLUDE_PATH)/ \
				-I$(PWD)/../../../prebuild/include \
				-I$(PWD)/../../../prebuild/include/exports \
				-I$(PWD)/../../../prebuild/include/imports \
				-I$(PWD)/../../../prebuild/include/kernel/rhino/core/include \
				-I$(PWD)/../../../prebuild/include/framework/netmgr/include \
				-I$(PWD)/../../../prebuild/include/platform/arch/arm/armv7m/gcc/m4 \
				-I$(PWD)/../../../prebuild/include/board/$(BOARD) \
				-I$(PWD)/../../../Living_SDK/kernel/protocols/net/include/ 

app = $(wildcard ./*.c)
obj_app = $(patsubst %.c,%.o,$(app))


CROSS_PREFIX="arm-none-eabi-" 
DEPENDS="" 
CCLD="" 
LDFLAGS="" 
CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld
AR=arm-none-eabi-ar
STRIP=arm-none-eabi-strip
OBJCOPY=arm-none-eabi-objcopy

# CFLAGS += -MD -ggdb -Os -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -std=gnu11 -Werror \
# 		  -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w

CFLAGS += -DWITH_LWIP -DCONFIG_AOS_CLI -DLOG_SIMPLE
CFLAGS += ${GLOBAL_CFLAGS}
CFLAGS += ${SDK_DEFINES}

ifeq ("$(BOARD)", "hf-lpt230")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DHF_LPT230
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/arch
else ifeq ("$(BOARD)", "hf-lpt130")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DHF_LPT130
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/arch
else ifeq ("$(BOARD)", "hf-lpb130")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DHF_LPB130
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/arch
else ifeq ("$(BOARD)", "hf-lpb135")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DHF_LPB135
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/arch
else ifeq ("$(BOARD)", "uno-91h")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DUNO_91H
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rda5981x/wifi/inc/arch
else ifeq ("$(BOARD)", "mk1101")
else ifeq ("$(BOARD)", "mk3060")
	CFLAGS +=  -mcpu=arm968e-s -march=armv5te -mthumb -mthumb-interwork -mlittle-endian -w -Os -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -std=gnu11 
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/lwip-2.0.2/port/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/lwip-2.0.2/port/arch \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/common/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/ip/common \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/driver/include \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/driver/common
else ifeq ("$(BOARD)", "mk3061")
	CFLAGS +=  -mcpu=arm968e-s -march=armv5te -mthumb -mthumb-interwork -mlittle-endian -w -Os -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -std=gnu11 
	CFLAGS += -DMK3061
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/lwip-2.0.2/port/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/lwip-2.0.2/port/arch \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/common/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/ip/common \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/driver/include \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/moc108/include/driver/common

else ifeq ("$(BOARD)", "mk3080")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DMK3080
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/arch
else ifeq ("$(BOARD)", "amebaz_dev")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DAMEBAZ_DEV
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/arch
else ifeq ("$(BOARD)", "mk3092")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DMK3092
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/arch
else ifeq ("$(BOARD)", "mk5080")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -march=armv7-m -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -w
	CFLAGS += -DMK5080
	INCLUDE += -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/ \
		   -I$(PWD)/../../../Living_SDK/platform/mcu/rtl8710bn/arch


else ifeq ("$(BOARD)", "asr5501")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -mfpu=fpv4-sp-d16 -std=gnu11 -w
	CFLAGS += -DASR5501
	INCLUDE += -I$(PWD)/../../../Living_SDK/board/asr5501/drivers/include/lwip_if/ \
		   -I$(PWD)/../../../Living_SDK/board/asr5501/drivers/include/lwip_if/arch/
else ifeq ("$(BOARD)", "mx1270")
	CFLAGS += -Os -Werror -fdata-sections -ffunction-sections -mcpu=cortex-m4 -mfloat-abi=soft -mlittle-endian -mthumb -mthumb-interwork -mfpu=fpv4-sp-d16 -std=gnu11 -w
	CFLAGS += -DMX1270
	INCLUDE += -I$(PWD)/../../../Living_SDK/board/mx1270/drivers/include/lwip_if/ \
		   -I$(PWD)/../../../Living_SDK/board/mx1270/drivers/include/lwip_if/arch/

else ifeq ("$(BOARD)", "bk7231devkitc")
    CFLAGS +=  -mcpu=arm968e-s -march=armv5te -mthumb -mthumb-interwork -mlittle-endian -w -Os -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -std=gnu11
    CFLAGS += -DBK7231DEVKITC
else ifeq ("$(BOARD)", "bk7231sdevkitc")
    CFLAGS +=  -mcpu=arm968e-s -march=armv5te -mthumb -mthumb-interwork -mlittle-endian -w -Os -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -std=gnu11
    CFLAGS += -DBK7231SDEVKITC
else ifeq ("$(BOARD)", "bk7231udevkitc")
    CFLAGS +=  -mcpu=arm968e-s -march=armv5te -mthumb -mthumb-interwork -mlittle-endian -w -Os -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -std=gnu11
    CFLAGS += -DBK7231UDEVKITC
else ifeq ("$(BOARD)", "b_l475e")

else ifeq ("$(BOARD)", "eml3047")
else ifeq ("$(BOARD)", "frdmkl27z")
else ifeq ("$(BOARD)", "lpcxpresso54102")

else ifeq ("$(BOARD)", "pca10056")

##  toolchains['arm-rockchip-linux-gnueabihf']
else ifeq ("$(BOARD)", "rk1108")

##  toolchains['nds32le-elf-newlib-v3']
else ifeq ("$(BOARD)", "sv6266_evb")
endif

ifneq ("$(BOARD)", "")
$(info BOARD=${BOARD} CONFIG_COMPILE_DATE=${CONFIG_COMPILE_DATE})
endif

# Compile date
CONFIG_COMPILE_DATE ?= $(shell date +%Y%m%d.%H%M%S)
all : $(target)

$(target):$(obj_app)
	@mkdir -p obj
	@mv *.o obj
	@$(RM) -rf *.d 
	@$(AR) -rcs $(PWD)/../../../prebuild/lib/$(target).a obj/*.o 2>/dev/null
	@$(RM) -rf obj

%.o:%.c
	@$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $< 

clean:
	@-$(RM)  obj/*
