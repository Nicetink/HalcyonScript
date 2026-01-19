@echo off
REM HalcyonScript-Native Full Build Script
REM Builds HalcyonScript with HalGUI and HalForms support
REM Usage: build_all.bat

set GCC=C:\msys64\mingw64\bin\gcc.exe
set AR=C:\msys64\mingw64\bin\ar.exe
set WINDRES=C:\msys64\mingw64\bin\windres.exe
set CFLAGS=-Wall -O2 -I. -Isrc -Isrc/halgui -Isrc/halforms
set LDFLAGS=-lgdi32 -lcomdlg32 -lole32 -luuid -lshell32 -lcomctl32

echo ========================================
echo Building HalcyonScript-Native
echo with HalGUI and HalForms support
echo ========================================

if not exist build mkdir build
if not exist dist mkdir dist

echo.
echo [1/5] Building HalGUI library...
%GCC% %CFLAGS% -c src/halgui/halgui_core.c -o build/halgui_core.o
%GCC% %CFLAGS% -c src/halgui/halgui_render.c -o build/halgui_render.o
%GCC% %CFLAGS% -c src/halgui/halgui_widgets.c -o build/halgui_widgets.o
%GCC% %CFLAGS% -c src/halgui/halgui_dialogs.c -o build/halgui_dialogs.o
%GCC% %CFLAGS% -c src/halgui/halgui_themes.c -o build/halgui_themes.o
%GCC% %CFLAGS% -c src/halgui/halgui_native.c -o build/halgui_native.o
%GCC% %CFLAGS% -c src/halgui/halgui_audio.c -o build/halgui_audio.o
%GCC% %CFLAGS% -c src/halgui/halgui_extended.c -o build/halgui_extended.o
%GCC% %CFLAGS% -c src/halgui/halgui_extended_render.c -o build/halgui_extended_render.o
if errorlevel 1 goto error
if errorlevel 1 goto error

echo [2/5] Building HalForms library...
%GCC% %CFLAGS% -c src/halforms/halforms_core.c -o build/halforms_core.o
%GCC% %CFLAGS% -c src/halforms/halforms_controls.c -o build/halforms_controls.o
%GCC% %CFLAGS% -c src/halforms/halforms_menu.c -o build/halforms_menu.o
%GCC% %CFLAGS% -c src/halforms/halforms_advanced.c -o build/halforms_advanced.o
%GCC% %CFLAGS% -c src/halforms/halforms_dialogs.c -o build/halforms_dialogs.o
%GCC% %CFLAGS% -c src/halforms/halforms_runtime.c -o build/halforms_runtime.o
%GCC% %CFLAGS% -c src/halforms/halforms_extended.c -o build/halforms_extended.o
%GCC% %CFLAGS% -c src/halforms/halforms_graphics.c -o build/halforms_graphics.o
%GCC% %CFLAGS% -c src/halforms/halforms_data.c -o build/halforms_data.o
if errorlevel 1 goto error

echo [3/5] Building HalcyonScript core...
%GCC% %CFLAGS% -c src/main.c -o build/main.o
%GCC% %CFLAGS% -c src/token.c -o build/token.o
%GCC% %CFLAGS% -c src/lexer.c -o build/lexer.o
%GCC% %CFLAGS% -c src/ast.c -o build/ast.o
%GCC% %CFLAGS% -c src/parser.c -o build/parser.o
%GCC% %CFLAGS% -c src/value.c -o build/value.o
%GCC% %CFLAGS% -c src/runtime.c -o build/runtime.o
%GCC% %CFLAGS% -c src/gui.c -o build/gui.o
%GCC% %CFLAGS% -c src/project.c -o build/project.o
%GCC% %CFLAGS% -c src/halgui_runtime.c -o build/halgui_runtime.o
%GCC% %CFLAGS% -c src/halgui_gpu_stub.c -o build/halgui_gpu_stub.o
%GCC% %CFLAGS% -c src/builtin_api.c -o build/builtin_api.o
%GCC% %CFLAGS% -c src/filesystem_api.c -o build/filesystem_api.o
%GCC% %CFLAGS% -c src/registry_api.c -o build/registry_api.o
%GCC% %CFLAGS% -c src/process_api.c -o build/process_api.o
%GCC% %CFLAGS% -c src/console_api.c -o build/console_api.o
%GCC% %CFLAGS% -c src/compression_api.c -o build/compression_api.o
if errorlevel 1 goto error

echo [4/5] Compiling resources...
REM Create resource file with manifest for modern visual styles
echo 1 24 "halcyon.manifest" > build\manifest.rc
%WINDRES% build\manifest.rc -o build\manifest_res.o
if errorlevel 1 (
    echo Warning: Manifest compilation failed
    set MANIFEST_OBJ=
) else (
    set MANIFEST_OBJ=build/manifest_res.o
)

if exist resources\app.rc (
    %WINDRES% resources\app.rc -o build\app_res.o
    if errorlevel 1 (
        echo Warning: Resource compilation failed, building without icon
        set RES_OBJ=
    ) else (
        set RES_OBJ=build/app_res.o
    )
) else (
    set RES_OBJ=
)

echo [5/5] Linking HalcyonRT.exe...
%GCC% %CFLAGS% ^
    build/main.o ^
    build/token.o ^
    build/lexer.o ^
    build/ast.o ^
    build/parser.o ^
    build/value.o ^
    build/runtime.o ^
    build/gui.o ^
    build/project.o ^
    build/halgui_runtime.o ^
    build/halgui_gpu_stub.o ^
    build/builtin_api.o ^
    build/filesystem_api.o ^
    build/registry_api.o ^
    build/process_api.o ^
    build/console_api.o ^
    build/compression_api.o ^
    build/halgui_core.o ^
    build/halgui_render.o ^
    build/halgui_widgets.o ^
    build/halgui_dialogs.o ^
    build/halgui_themes.o ^
    build/halgui_native.o ^
    build/halgui_audio.o ^
    build/halgui_extended.o ^
    build/halgui_extended_render.o ^
    build/halforms_core.o ^
    build/halforms_controls.o ^
    build/halforms_menu.o ^
    build/halforms_advanced.o ^
    build/halforms_dialogs.o ^
    build/halforms_runtime.o ^
    build/halforms_extended.o ^
    build/halforms_graphics.o ^
    build/halforms_data.o ^
    %RES_OBJ% ^
    %MANIFEST_OBJ% ^
    %LDFLAGS% -lwinmm -lgdiplus -luxtheme -lmsimg32 ^
    -o HalcyonRT.exe

if errorlevel 1 goto error

echo.
echo Copying to dist...
copy /Y HalcyonRT.exe dist\ >nul
copy /Y halcyon.manifest dist\ >nul

echo.
echo ========================================
echo Build successful!
echo Output: HalcyonRT.exe
echo        dist\HalcyonRT.exe
echo ========================================
echo.
echo Supported UI frameworks:
echo   - HalGUI  (modern, themed UI)
echo   - HalForms (Windows Forms-like, native controls)
echo.
goto end

:error
echo.
echo Build failed!
exit /b 1

:end
