@echo off
REM HalcyonScript-Native with HalGUI Build Script
REM Usage: build_halgui.bat

set GCC=C:\msys64\mingw64\bin\gcc.exe
set AR=C:\msys64\mingw64\bin\ar.exe
set WINDRES=C:\msys64\mingw64\bin\windres.exe
set CFLAGS=-Wall -O2 -I. -Isrc -Isrc/halgui
set LDFLAGS=-lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32

echo ========================================
echo Building HalcyonScript-Native with HalGUI
echo ========================================

echo.
echo [1/3] Building HalGUI library...
%GCC% %CFLAGS% -c src/halgui/halgui_core.c -o build/halgui_core.o
%GCC% %CFLAGS% -c src/halgui/halgui_render.c -o build/halgui_render.o
%GCC% %CFLAGS% -c src/halgui/halgui_widgets.c -o build/halgui_widgets.o
%GCC% %CFLAGS% -c src/halgui/halgui_dialogs.c -o build/halgui_dialogs.o
%GCC% %CFLAGS% -c src/halgui/halgui_themes.c -o build/halgui_themes.o
%GCC% %CFLAGS% -c src/halgui/halgui_native.c -o build/halgui_native.o
%GCC% %CFLAGS% -c src/halgui/halgui_audio.c -o build/halgui_audio.o
if errorlevel 1 goto error

echo [2/3] Building HalcyonScript core...
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
if errorlevel 1 goto error

echo [3/3] Compiling resources and linking halcyon.exe...
if exist resources\app.rc (
    %WINDRES% resources\app.rc -o build\app_res.o
    if errorlevel 1 (
        echo Warning: Resource compilation failed, building without icon
        %GCC% %CFLAGS% build/*.o %LDFLAGS% -lwinmm -lgdiplus -o halcyon.exe
    ) else (
        %GCC% %CFLAGS% build/*.o build/app_res.o %LDFLAGS% -lwinmm -lgdiplus -o halcyon.exe
    )
) else (
    %GCC% %CFLAGS% build/*.o %LDFLAGS% -lwinmm -lgdiplus -o halcyon.exe
)
if errorlevel 1 goto error

echo.
echo ========================================
echo Build successful!
echo Output: halcyon.exe
echo ========================================
goto end

:error
echo.
echo Build failed!
exit /b 1

:end
