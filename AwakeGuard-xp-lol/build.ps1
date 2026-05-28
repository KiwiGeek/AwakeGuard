# Build AwakeGuard-XP-LOL with MSYS2 MinGW i686 (must be on PATH).
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

$gcc = Get-Command i686-w64-mingw32-g++ -ErrorAction SilentlyContinue
if (-not $gcc) {
    Write-Error "i686-w64-mingw32-g++ not found. Install MSYS2: pacman -S mingw-w64-i686-gcc mingw-w64-i686-cmake make"
}

if (-not (Test-Path "$root\..\assets\AwakeGuard.ico")) {
    Write-Host "Generating icons..."
    python "$root\..\scripts\generate-icons.py"
}

$buildDir = Join-Path $root "build"
cmake -S $root -B $buildDir -G "MinGW Makefiles" `
    -DCMAKE_TOOLCHAIN_FILE="$root\cmake\mingw-i686-xp.cmake" `
    -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc `
    -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++
cmake --build $buildDir

$exe = Join-Path $buildDir "AwakeGuard-XP-LOL.exe"
if (Test-Path $exe) {
    Write-Host ""
    Write-Host "Built: $exe"
} else {
    Write-Error "Build finished but exe not found."
}
