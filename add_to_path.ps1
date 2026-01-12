# Add HalcyonScript to System PATH
# Run as Administrator: powershell -ExecutionPolicy Bypass -File add_to_path.ps1

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Add HalcyonScript to System PATH" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Get the directory where halcyon.exe is located
$halcyonDir = Join-Path $PSScriptRoot "dist"
$halcyonExe = Join-Path $halcyonDir "halcyon.exe"

# Check if halcyon.exe exists
if (-not (Test-Path $halcyonExe)) {
    Write-Host "ERROR: halcyon.exe not found at $halcyonExe" -ForegroundColor Red
    Write-Host "Please build HalcyonScript first using build.bat" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "HalcyonScript found at: $halcyonDir" -ForegroundColor Green
Write-Host ""

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script must be run as Administrator" -ForegroundColor Red
    Write-Host ""
    Write-Host "Right-click PowerShell and select 'Run as Administrator', then run:" -ForegroundColor Yellow
    Write-Host "  powershell -ExecutionPolicy Bypass -File add_to_path.ps1" -ForegroundColor Yellow
    Write-Host ""
    Read-Host "Press Enter to exit"
    exit 1
}

# Get current system PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")

# Check if already in PATH
if ($currentPath -like "*$halcyonDir*") {
    Write-Host "HalcyonScript is already in system PATH!" -ForegroundColor Yellow
    Write-Host ""
} else {
    # Add to PATH
    Write-Host "Adding to system PATH..." -ForegroundColor Cyan
    $newPath = $currentPath + ";" + $halcyonDir
    [Environment]::SetEnvironmentVariable("Path", $newPath, "Machine")
    
    Write-Host ""
    Write-Host "SUCCESS! HalcyonScript has been added to system PATH." -ForegroundColor Green
    Write-Host ""
}

Write-Host "IMPORTANT: You need to restart any open terminals or IDEs" -ForegroundColor Yellow
Write-Host "for the PATH changes to take effect." -ForegroundColor Yellow
Write-Host ""
Write-Host "You can now run: halcyon script.hcs" -ForegroundColor Green
Write-Host ""

# Test the installation
Write-Host "Testing installation..." -ForegroundColor Cyan
$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine")
try {
    $version = & $halcyonExe version 2>&1
    Write-Host "Test successful!" -ForegroundColor Green
} catch {
    Write-Host "Note: Restart your terminal to use 'halcyon' command" -ForegroundColor Yellow
}

Write-Host ""
Read-Host "Press Enter to exit"
