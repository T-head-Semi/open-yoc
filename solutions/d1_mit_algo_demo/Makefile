CPRE := @
ifeq ($(V),1)
CPRE :=
VERB := --verbose
endif

MK_GENERATED_IMGS_PATH:=generated
ALGORITHMS_PATH:=../../components/chip_d1/d1_flash.elf

.PHONY:startup
startup: all

all:
	@echo "Build Solution by $(BOARD) "
	$(CPRE) scons $(VERB) --board=$(BOARD) -j4
	@echo YoC SDK Done

	@echo [INFO] Create bin files
	$(CPRE) bash packimg.sh

.PHONY:flashall
flashall:
	$(CPRE) product flash ${MK_GENERATED_IMGS_PATH}/images.zip -w prim -x gdbinit -f ${ALGORITHMS_PATH}
	$(CPRE) product flash ${MK_GENERATED_IMGS_PATH}/images.zip -w boot -x gdbinit -f ${ALGORITHMS_PATH}
	$(CPRE) product flash ${MK_GENERATED_IMGS_PATH}/images.zip -w lfs -x gdbinit -f ${ALGORITHMS_PATH}

.PHONY:flash
flash:
	$(CPRE) product flash ${MK_GENERATED_IMGS_PATH}/images.zip -w prim -x gdbinit -f ${ALGORITHMS_PATH}


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
