@echo off
setlocal enabledelayedexpansion

set "VS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VS%" (
    for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath') do set "VSROOT=%%i"
    set "VS=!VSROOT!\VC\Auxiliary\Build\vcvars64.bat"
)

call "%VS%" >nul
if errorlevel 1 (
    echo [build] vcvars64 not found
    exit /b 1
)

cd /d "%~dp0"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build
if errorlevel 1 exit /b 1

echo.
echo [build] OK - nayclient.dll and naylauncher.exe in build\
endlocal
