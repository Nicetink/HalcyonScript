@echo off
REM Build HalcyonScript-Native with HalGUI and install to dist

echo ========================================
echo HalcyonScript-Native Build and Install
echo ========================================

REM Create build directory
if not exist build mkdir build

REM Create dist directory
if not exist dist mkdir dist

REM Build
call build_halgui.bat
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

REM Copy to dist
echo.
echo Installing to dist folder...
copy /Y halcyon.exe dist\halcyon.exe

echo.
echo ========================================
echo Installation complete!
echo Location: %CD%\dist\halcyon.exe
echo ========================================
echo.
echo You can now run HalcyonScript files:
echo   dist\halcyon.exe examples\halgui_simple.hcs
echo.
