@echo off
echo ===================================
echo HalcyonScript Build
echo ===================================

where gcc >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo Error: GCC not found. Install MSYS2 MinGW-w64.
    exit /b 1
)

if not exist dist mkdir dist
if not exist build mkdir build

echo Compiling resources...

REM Compile resource file with icon
windres resources/app.rc -o build/app_res.o 2>nul
if %ERRORLEVEL% neq 0 (
    echo Warning: Resource compilation failed, building without icon
    set RES_OBJ=
) else (
    echo   + halcyon.ico embedded
    set RES_OBJ=build/app_res.o
)

echo Compiling...

gcc -Wall -O2 -I./include -I./src/halgui ^
    src/main.c ^
    src/token.c ^
    src/lexer.c ^
    src/ast.c ^
    src/parser.c ^
    src/value.c ^
    src/runtime.c ^
    src/project.c ^
    src/halgui_runtime.c ^
    src/halgui_gpu_stub.c ^
    src/gui.c ^
    %RES_OBJ% ^
    -o dist/halcyon.exe ^
    -L./src/halgui -lhalgui ^
    -mwindows ^
    -lcomctl32 -lgdi32 -lshell32 -ld3d11 -ldxgi -lgdiplus

if %ERRORLEVEL% equ 0 (
    echo.
    echo Build successful: dist\halcyon.exe
    dir dist\halcyon.exe
    
    echo.
    echo Copying launcher files...
    if not exist dist\launcher mkdir dist\launcher
    copy /Y src\launcher\launcher.c dist\launcher\ >nul
    copy /Y src\launcher\launcher.rc dist\launcher\ >nul
    copy /Y src\launcher\build_launcher.bat dist\launcher\ >nul
    echo Launcher files copied to dist\launcher\
    
    echo.
    echo Copying logo files...
    if not exist dist\logo mkdir dist\logo
    copy /Y logo\halcyon.ico dist\logo\ >nul
    copy /Y logo\halcyon.ico dist\ >nul
    echo Logo files copied to dist\logo\ and dist\
) else (
    echo Build failed!
    exit /b 1
)
