@echo off
REM Add HalcyonScript to System PATH
REM Run as Administrator

echo ========================================
echo  Add HalcyonScript to System PATH
echo ========================================
echo.

REM Get the current directory where halcyon.exe is located
set "HALCYON_DIR=%~dp0dist"

REM Check if halcyon.exe exists
if not exist "%HALCYON_DIR%\halcyon.exe" (
    echo ERROR: halcyon.exe not found in %HALCYON_DIR%
    echo Please build HalcyonScript first using build.bat
    pause
    exit /b 1
)

echo HalcyonScript found at: %HALCYON_DIR%
echo.

REM Add to system PATH (requires admin)
echo Adding to system PATH...
setx PATH "%PATH%;%HALCYON_DIR%" /M

if %ERRORLEVEL% EQU 0 (
    echo.
    echo SUCCESS! HalcyonScript has been added to system PATH.
    echo.
    echo IMPORTANT: You need to restart any open terminals or IDEs
    echo for the PATH changes to take effect.
    echo.
    echo You can now run: halcyon script.hcs
) else (
    echo.
    echo ERROR: Failed to add to PATH.
    echo Please run this script as Administrator.
    echo.
    echo Alternative: Manually add this folder to PATH:
    echo   %HALCYON_DIR%
)

echo.
pause
