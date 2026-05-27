@echo off
setlocal

rem Usage:
rem   build.bat              -> Release | x64
rem   build.bat x64          -> Release | x64
rem   build.bat Win32        -> Release | Win32   (32-bit)
rem   build.bat ARM64        -> Release | ARM64

set "PLATFORM=%~1"
if /i "%PLATFORM%"==""        set "PLATFORM=x64"
if /i "%PLATFORM%"=="x86"     set "PLATFORM=Win32"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Could not find Visual Studio. Install "Desktop development with C++" from:
    echo https://visualstudio.microsoft.com/downloads/
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
if not defined MSBUILD (
    echo Could not find MSBuild. Install Visual Studio with C++ workload.
    exit /b 1
)

"%MSBUILD%" "%~dp0AwakeGuard.sln" /p:Configuration=Release /p:Platform=%PLATFORM% /m
if errorlevel 1 exit /b 1

echo.
echo Built: %~dp0bin\%PLATFORM%\Release\AwakeGuard.exe
