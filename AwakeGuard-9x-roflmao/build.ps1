# Build AwakeGuard-9x-ROFLMAO with MSYS2 MinGW i686.
#Requires -Version 5.1
param(
    [switch]$InstallMsys2,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

function Find-Msys2Root {
    $candidates = @(
        $env:MSYS2_ROOT,
        "C:\msys64",
        "$env:LOCALAPPDATA\msys64",
        "$env:ProgramFiles\msys64"
    ) | Where-Object { $_ -and (Test-Path $_) }

    foreach ($base in $candidates) {
        if (Test-Path (Join-Path $base "mingw32\bin\i686-w64-mingw32-g++.exe")) {
            return $base
        }
    }
    return $null
}

function Resolve-MingwMake([string]$mingwBin) {
    foreach ($name in @("mingw32-make.exe", "make.exe")) {
        $path = Join-Path $mingwBin $name
        if (Test-Path $path) {
            return $path
        }
    }
    return $null
}

function Resolve-Windres([string]$mingwBin) {
    foreach ($name in @("windres.exe", "i686-w64-mingw32-windres.exe")) {
        $path = Join-Path $mingwBin $name
        if (Test-Path $path) {
            return $path
        }
    }
    return $null
}

function Show-Msys2InstallHelp {
    Write-Host ""
    Write-Host "MSYS2 + MinGW 32-bit is required to cross-compile for Windows 9x/XP." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "1) Install MSYS2 (PowerShell):" -ForegroundColor Cyan
    Write-Host "     winget install -e --id MSYS2.MSYS2"
    Write-Host ""
    Write-Host "2) Open Start Menu -> MSYS2 MinGW 32-bit" -ForegroundColor Cyan
    Write-Host "   (NOT PowerShell - pacman only works there.)"
    Write-Host ""
    Write-Host "3) Install toolchain:" -ForegroundColor Cyan
    Write-Host "     pacman -Syu --noconfirm"
    Write-Host "     pacman -S --needed --noconfirm mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-make mingw-w64-i686-binutils"
    Write-Host ""
    Write-Host "4) Back in PowerShell:" -ForegroundColor Cyan
    Write-Host "     .\build.ps1"
    Write-Host ""
}

if ($InstallMsys2) {
    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if (-not $winget) {
        Write-Error "winget not found. Install MSYS2 manually from https://www.msys2.org/"
    }
    Write-Host "Installing MSYS2 via winget..."
    & winget install -e --id MSYS2.MSYS2 --accept-package-agreements --accept-source-agreements
    Write-Host "MSYS2 installed. Complete steps 2-4 from the help below."
    Show-Msys2InstallHelp
    exit 0
}

$msys2 = Find-Msys2Root
if (-not $msys2) {
    Show-Msys2InstallHelp
    Write-Error "MSYS2 mingw32 toolchain not found."
}

$mingwBin = Join-Path $msys2 "mingw32\bin"
$env:PATH = "$mingwBin;$env:PATH"
Write-Host "Using MinGW from: $mingwBin"

$gcc = Join-Path $mingwBin "i686-w64-mingw32-g++.exe"
if (-not (Test-Path $gcc)) {
    Show-Msys2InstallHelp
    Write-Error "i686-w64-mingw32-g++ not found."
}

$cmakeExe = Join-Path $mingwBin "cmake.exe"
if (-not (Test-Path $cmakeExe)) {
    Show-Msys2InstallHelp
    Write-Error "cmake.exe not found in mingw32\bin."
}

$windresExe = Resolve-Windres $mingwBin
if (-not $windresExe) {
    Write-Error "windres not found in $mingwBin"
}

$makeExe = Resolve-MingwMake $mingwBin
if (-not $makeExe) {
    Write-Error "mingw32-make not found in $mingwBin"
}

Write-Host "Using make: $makeExe"
Write-Host "Using windres: $windresExe"
Write-Host "Using cmake: $cmakeExe"

if (-not (Test-Path "$root\..\assets\AwakeGuard.ico")) {
    Write-Host "Generating icons..."
    python "$root\..\scripts\generate-icons.py"
}

$buildDir = Join-Path $root "build"
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning $buildDir ..."
    try {
        Remove-Item -Recurse -Force $buildDir -ErrorAction Stop
    } catch {
        Write-Host "Could not delete build folder. Close AwakeGuard-9x-ROFLMAO.exe, then: .\build.ps1 -Clean" -ForegroundColor Yellow
        throw
    }
}

$version = "0.1.1"
if ($env:AWAKEGUARD_VERSION) {
    $version = $env:AWAKEGUARD_VERSION
}

& $cmakeExe -S $root -B $buildDir -G "MinGW Makefiles" `
    -DCMAKE_MAKE_PROGRAM="$makeExe" `
    -DCMAKE_RC_COMPILER="$windresExe" `
    -DCMAKE_TOOLCHAIN_FILE="$root\cmake\mingw-i686-9x.cmake" `
    -DCMAKE_C_COMPILER="$mingwBin\i686-w64-mingw32-gcc.exe" `
    -DCMAKE_CXX_COMPILER="$mingwBin\i686-w64-mingw32-g++.exe" `
    -DAWAKEGUARD_VERSION="$version"

& $cmakeExe --build $buildDir

$exe = Join-Path $buildDir "AwakeGuard-9x-ROFLMAO.exe"
if (Test-Path $exe) {
    Write-Host ""
    Write-Host "Built: $exe" -ForegroundColor Green
} else {
    Write-Error "Build finished but exe not found."
}
