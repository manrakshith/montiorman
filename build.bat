@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo        Building Native MonitorMan         
echo ==========================================

:: Add MinGW to PATH if not already present
if exist "C:\Qt\Tools\mingw1310_64\bin\g++.exe" (
    set "PATH=C:\Qt\Tools\mingw1310_64\bin;!PATH!"
)

:: Check for compiler
where g++.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] g++ compiler not found in PATH or C:\Qt\Tools\mingw1310_64\bin.
    exit /b 1
)

if not exist "bin" mkdir "bin"

echo Compiling Windows Resources (Manifest, DPI)...
windres.exe -i res/resource.rc -o bin/resource.o --input-format=rc --output-format=coff
if %errorlevel% neq 0 (
    echo [WARNING] windres failed or skipped, compiling without embedded manifest.
    set "RES_OBJ="
) else (
    set "RES_OBJ=bin/resource.o"
)

echo Compiling C++ executable...
g++.exe -std=c++17 -O2 -s -mwindows ^
    src/main.cpp !RES_OBJ! ^
    -o bin/monitorman.exe ^
    -static -static-libgcc -static-libstdc++ ^
    -ldxva2 -ldwmapi -luxtheme -lcomctl32 -lgdi32 -luser32 -lshell32

if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo [SUCCESS] Built successfully: bin\monitorman.exe
for %%I in (bin\monitorman.exe) do (
    set /a size_kb=%%~zI / 1024
    echo Executable Size: %%~zI bytes (!size_kb! KB^)
)
echo.
