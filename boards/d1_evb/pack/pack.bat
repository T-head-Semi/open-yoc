@echo off
setlocal enableDelayedExpansion

set CURDIR=%~dp0
set RTOS_IMG=%1
set FS_IMG=%2
set OUT_DIR=%3
set MK_GENERATED_IMGS_PATH=%4
set SYS_PARTITION_FILE=%5

echo packing the image...
@REM echo "%CURDIR%"
@REM echo "%RTOS_IMG% %FS_IMG% %OUT_DIR%"
@REM export MELIS_BASE=${CURDIR}


set PRODUCT="%CURDIR%..\configs\product.exe"

if not exist %PRODUCT% (
    echo "the product.exe is not found."
    exit /B 1
)

%PRODUCT% version

if "%SYS_PARTITION_FILE%" == "" (
    set SYS_PARTITION_FILE="%CURDIR%..\configs\sys_partition_nor.fex"
)

if not exist %SYS_PARTITION_FILE% (
    set SYS_PARTITION_FILE="%CURDIR%..\configs\sys_partition_nor.fex"
)

if not exist %SYS_PARTITION_FILE% (
    echo "the sys partition file is not exist."
    exit /B 1
)

@REM python %CURDIR%tools\packtool-win\pack.py 1 %RTOS_IMG% %FS_IMG% %OUT_DIR% %MK_GENERATED_IMGS_PATH% %SYS_PARTITION_FILE% %PRODUCT%
%CURDIR%tools\packtool-win\pack.exe 0 %RTOS_IMG% %FS_IMG% %OUT_DIR% %MK_GENERATED_IMGS_PATH% %SYS_PARTITION_FILE% %PRODUCT%
