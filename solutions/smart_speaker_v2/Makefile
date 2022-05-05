CPRE := @
ifeq ($(V),1)
CPRE :=
VERB := --verbose
endif

MK_GENERATED_IMGS_PATH:=generated
ALGORITHMS_PATH:=../../components/chip_d1/d1_flash.elf

cpunum = $(shell cat /proc/cpuinfo| grep "processor"| wc -l)

.PHONY:startup
startup: all

all:
	@echo "Build Solution by $(BOARD) "
	$(CPRE) scons $(VERB) --board=$(BOARD) -j$(cpunum)
	@echo YoC SDK Done

	@echo [INFO] Create bin files
	$(CPRE) bash packimg.sh

.PHONY:map
map:
	python ./tools/debug/map_parse_xls_v1.5.py yoc.map

APP_VERSION := $(shell date "+%m.%d.%H")
RLS_DIR := Voice_App_Platform_V2.0_$(APP_VERSION)
GIT_COMMITID := $(shell git rev-parse HEAD)
rls:
	rm -fr Voice_App_Platform_V2.0.*
	mkdir -p $(RLS_DIR)

	cp out/yoc_rtos_8M.img $(RLS_DIR)
	cp yoc.elf $(RLS_DIR)
	echo $(APP_VERSION) $(GIT_COMMITID) > $(RLS_DIR)/appversion
	cp yoc.map $(RLS_DIR)

	cp tools/debug/coredump_parser_mmleak.py $(RLS_DIR)
	cp tools/debug/mmleak.sh $(RLS_DIR)

	zip -r $(RLS_DIR).zip $(RLS_DIR)
	rm -fr $(RLS_DIR)


.PHONY:flashall
flashall:
	$(CPRE) product flash ${MK_GENERATED_IMGS_PATH}/images.zip -a -x gdbinit -f ${ALGORITHMS_PATH}

.PHONY:flash
flash:
	$(CPRE) product flash ${MK_GENERATED_IMGS_PATH}/images.zip -w prim -x gdbinit -f ${ALGORITHMS_PATH}

.PHONY:flashfs
flashfs:
	$(CPRE) product flash ${MK_GENERATED_IMGS_PATH}/images.zip -w lfs -x gdbinit -f ${ALGORITHMS_PATH}

sdk:
	$(CPRE) yoc sdk

.PHONY:clean
clean:
	$(CPRE) scons -c
ifeq ($(OS), Windows_NT)
	$(CPRE) if exist yoc_sdk rmdir /s /q yoc_sdk
	$(CPRE) if exist binary rmdir /s /q binary
	$(CPRE) if exist out rmdir /s /q out
	$(CPRE) if exist generated rmdir /s /q generated
	$(CPRE) if exist yoc.elf del /f /q yoc.elf
	$(CPRE) if exist yoc.map del /f /q yoc.map
else
	$(CPRE) rm -rf yoc_sdk binary out yoc.elf yoc.map generated
endif
