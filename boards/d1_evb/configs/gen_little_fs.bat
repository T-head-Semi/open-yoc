@echo off
setlocal enableDelayedExpansion

echo generate littlefs.bin
set SCRIPT_ABS_DIR=%~dp0

if exist data (
    %SCRIPT_ABS_DIR%\mklittlefs.exe -c data -d 5 -b 4096 -p 1792 -s 4530176 littlefs.bin >nul
) else (
    echo No data dir found,do not need to generate filesystem.
)
