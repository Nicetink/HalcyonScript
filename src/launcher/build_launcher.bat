@echo off
REM Build HalcyonScript Application Launcher
REM Usage: build_launcher.bat <output_name> <temp_dir>
REM
REM The launcher will use halcyon.ico as the default application icon.
REM This icon should be copied to the temp_dir before running this script.

setlocal enabledelayedexpansion

set OUTPUT_NAME=%1
set TEMP_DIR=%2
set SCRIPT_DIR=%~dp0

if "%OUTPUT_NAME%"=="" (
    echo Usage: build_launcher.bat ^<output_name^> ^<temp_dir^>
    exit /b 1
)

if "%TEMP_DIR%"=="" (
    echo Usage: build_launcher.bat ^<output_name^> ^<temp_dir^>
    exit /b 1
)

echo Building launcher: %OUTPUT_NAME%.exe

REM Check if icon exists
if not exist "%TEMP_DIR%\halcyon.ico" (
    echo Warning: halcyon.ico not found in %TEMP_DIR%
    echo The application will be built without an icon.
)

REM Compile resources
echo [1/3] Compiling resources...
windres "%TEMP_DIR%\launcher.rc" -o "%TEMP_DIR%\launcher_res.o"
if errorlevel 1 (
    echo Error: Failed to compile resources
    exit /b 1
)

REM Compile launcher
echo [2/3] Compiling launcher...
gcc -O2 -mwindows -o "%TEMP_DIR%\%OUTPUT_NAME%.exe" "%SCRIPT_DIR%launcher.c" "%TEMP_DIR%\launcher_res.o" -lshlwapi
if errorlevel 1 (
    echo Error: Failed to compile launcher
    exit /b 1
)

echo [3/3] Done!
echo Output: %TEMP_DIR%\%OUTPUT_NAME%.exe

exit /b 0
