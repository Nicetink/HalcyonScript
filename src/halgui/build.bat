@echo off
REM HalGUI Build Script for Windows (MinGW)
REM Usage: build.bat [demo]

set GCC=C:\msys64\mingw64\bin\gcc.exe
set AR=C:\msys64\mingw64\bin\ar.exe
set CFLAGS=-Wall -O2 -I.
set LDFLAGS=-lgdi32 -lcomdlg32 -lole32 -lshell32 -mwindows

echo Building HalGUI...

%GCC% %CFLAGS% -c halgui_core.c -o halgui_core.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c halgui_render.c -o halgui_render.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c halgui_widgets.c -o halgui_widgets.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c halgui_dialogs.c -o halgui_dialogs.o
if errorlevel 1 goto error

%GCC% %CFLAGS% -c halgui_themes.c -o halgui_themes.o
if errorlevel 1 goto error

echo Creating library...
%AR% rcs libhalgui.a halgui_core.o halgui_render.o halgui_widgets.o halgui_dialogs.o halgui_themes.o
if errorlevel 1 goto error

echo HalGUI library built successfully!

if "%1"=="demo" (
    echo Building demo...
    %GCC% %CFLAGS% ..\..\examples\halgui_demo.c -L. -lhalgui %LDFLAGS% -o ..\..\examples\halgui_demo.exe
    if errorlevel 1 goto error
    echo Demo built: examples\halgui_demo.exe
)

del *.o 2>nul
echo Done!
goto end

:error
echo Build failed!
exit /b 1

:end
