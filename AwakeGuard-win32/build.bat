@echo off
setlocal

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

"%MSBUILD%" "%~dp0AwakeGuard.sln" /p:Configuration=Release /p:Platform=x64 /m
if errorlevel 1 exit /b 1

echo.
echo Built: %~dp0bin\Release\AwakeGuard.exe
