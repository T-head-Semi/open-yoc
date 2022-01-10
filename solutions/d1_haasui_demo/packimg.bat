@echo off
setlocal enableDelayedExpansion

set CURDIR=%~dp0
set BOARD_DIR=..\..\boards\d1_evb
set PACK_DIR=%BOARD_DIR%\pack
set OUT_DIR=%CURDIR%out
set RTOS_IMG=%CURDIR%yoc.bin
set FS_IMG=%CURDIR%littlefs.bin
set MK_GENERATED_IMGS_PATH=%CURDIR%generated

@REM echo %CURDIR%
@REM echo %BOARD_DIR%
@REM echo %PACK_DIR%
@REM echo %OUT_DIR%
@REM echo %RTOS_IMG%
@REM echo %FS_IMG%
@REM echo %MK_GENERATED_IMGS_PATH%
@REM echo "---------------------------------------"

if not exist %OUT_DIR% (
    md %OUT_DIR%
)

if not exist %RTOS_IMG% (
    echo "file %RTOS_IMG% not found"
    exit /B 1
) else (
    if not exist %MK_GENERATED_IMGS_PATH% (
        md %MK_GENERATED_IMGS_PATH%\data
    )
    %BOARD_DIR%\configs\gen_little_fs.bat
    if not exist "%MK_GENERATED_IMGS_PATH%\data" (
        echo "folder %MK_GENERATED_IMGS_PATH%\data not found."
        exit /B 1
    )

    @REM copy /Y %BOARD_DIR%\bootimgs\boot0 %MK_GENERATED_IMGS_PATH%\data >/nul
    @REM copy /Y %BOARD_DIR%\bootimgs\boot %MK_GENERATED_IMGS_PATH%\data >/nul
    @REM copy /Y %BOARD_DIR%\configs\config.yaml %MK_GENERATED_IMGS_PATH%\data >/nul

    @REM echo the rtos image is %RTOS_IMG%
    %PACK_DIR%\pack.bat %RTOS_IMG% %FS_IMG% %OUT_DIR% %MK_GENERATED_IMGS_PATH%
)
