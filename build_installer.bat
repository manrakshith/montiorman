@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo       Building MonitorMan Installer       
echo ==========================================

:: 1. Rebuild executable first
call build.bat
if %errorlevel% neq 0 (
    echo [ERROR] Application build failed! Cannot create installer.
    exit /b 1
)

:: 2. Locate NSIS makensis.exe
set "MAKENSIS="
if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
    set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
) else if exist "C:\Program Files\NSIS\makensis.exe" (
    set "MAKENSIS=C:\Program Files\NSIS\makensis.exe"
) else (
    where makensis.exe >nul 2>&1
    if !errorlevel! equ 0 (
        set "MAKENSIS=makensis.exe"
    )
)

if not defined MAKENSIS (
    echo [ERROR] NSIS compiler was not found!
    echo Please install NSIS via winget: winget install NSIS.NSIS
    exit /b 1
)

echo Compiling NSIS Installer using "!MAKENSIS!"...
"!MAKENSIS!" /V2 installer.nsi

if %errorlevel% neq 0 (
    echo [ERROR] NSIS installer generation failed!
    exit /b 1
)

echo.
echo ==========================================
echo [SUCCESS] Installer Built Successfully!
echo ==========================================
if exist "bin\MonitorMan_Setup.exe" (
    for %%I in ("bin\MonitorMan_Setup.exe") do (
        set /a size_kb=%%~zI / 1024
        echo Output File : bin\MonitorMan_Setup.exe
        echo Package Size: %%~zI bytes (!size_kb! KB^)
    )
)
echo.
