@echo off
REM HalForms Build Script
REM Builds HalForms library for HalcyonScript
REM Usage: build_halforms.bat

set GCC=C:\msys64\mingw64\bin\gcc.exe
set AR=C:\msys64\mingw64\bin\ar.exe
set CFLAGS=-Wall -O2 -I. -Isrc -Isrc/halforms -DUNICODE -D_UNICODE
set LDFLAGS=-lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32 -lshlwapi

echo ========================================
echo Building HalForms Library
echo ========================================

if not exist build mkdir build
if not exist dist mkdir dist

echo.
echo [1/2] Compiling HalForms source files...

%GCC% %CFLAGS% -c src/halforms/halforms_core.c -o build/halforms_core.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_controls.c -o build/halforms_controls.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_menu.c -o build/halforms_menu.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_advanced.c -o build/halforms_advanced.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_dialogs.c -o build/halforms_dialogs.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_runtime.c -o build/halforms_runtime.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_scintilla.c -o build/halforms_scintilla.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_layout.c -o build/halforms_layout.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_process.c -o build/halforms_process.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_ide_api.c -o build/halforms_ide_api.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c src/halforms/halforms_runtime_ext.c -o build/halforms_runtime_ext.o
if errorlevel 1 goto error

echo [2/2] Creating static library libhalforms.a...

%AR% rcs src/halforms/libhalforms.a ^
    build/halforms_core.o ^
    build/halforms_controls.o ^
    build/halforms_menu.o ^
    build/halforms_advanced.o ^
    build/halforms_dialogs.o ^
    build/halforms_runtime.o ^
    build/halforms_scintilla.o ^
    build/halforms_layout.o ^
    build/halforms_process.o ^
    build/halforms_ide_api.o ^
    build/halforms_runtime_ext.o

if errorlevel 1 goto error

copy /Y src\halforms\libhalforms.a dist\ >nul

echo.
echo ========================================
echo HalForms build successful!
echo Output: src/halforms/libhalforms.a
echo        dist/libhalforms.a
echo ========================================
goto end

:error
echo.
echo HalForms build failed!
exit /b 1

:end
