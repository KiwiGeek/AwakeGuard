#Requires -Version 5.1
<#
.SYNOPSIS
  Stage a 1.44M floppy image for Win95 VMs (AwakeGuard 9x ROFLMAO).

.DESCRIPTION
  The ROFLMAO build links MSVCRT20.DLL (stock Win95 System), so only the exe is staged.
  Use -IncludeMsvcrt only if you rebuilt with the default MinGW msvcrt target.
#>
param(
    [string]$ExePath = (Join-Path $PSScriptRoot "..\build\AwakeGuard-9x-ROFLMAO.exe"),
    [string]$OutDir = (Join-Path $PSScriptRoot "..\floppy"),
    [switch]$NoStrip,
    [switch]$IncludeMsvcrt,
    [string]$MsvcrtPath = (Join-Path $PSScriptRoot "..\redist\MSVCRT.DLL"),
    [string]$DiskImageJs
)

$ErrorActionPreference = "Stop"
$FloppyMax = 1474560
$floppyRoot = (Resolve-Path (New-Item -ItemType Directory -Force -Path $OutDir)).Path
$disk1 = Join-Path $floppyRoot "disk1"
$stripDir = Join-Path $floppyRoot "_stage"

if (-not (Test-Path $ExePath)) {
    Write-Error "Build the exe first: $ExePath"
}

$imports = & C:\msys64\mingw32\bin\objdump.exe -p $ExePath 2>&1 | Select-String "DLL Name:"
if ($imports -match "msvcrt\.dll" -and -not $IncludeMsvcrt) {
    Write-Warning @"
$ExePath imports MSVCRT.DLL. Rebuild with current cmake/mingw-i686-9x.cmake (-mcrtdll=msvcrt20),
or rerun with -IncludeMsvcrt and a VC6 MSVCRT.DLL (see redist\README.md).
"@
}

$strip = "C:\msys64\mingw32\bin\strip.exe"
$workExe = $ExePath
if (-not $NoStrip) {
    if (-not (Test-Path $strip)) {
        Write-Error "strip.exe not found at $strip (install MSYS2 mingw32 or pass -NoStrip)"
    }
    New-Item -ItemType Directory -Force -Path $stripDir | Out-Null
    $workExe = Join-Path $stripDir "AWAKEG.EXE"
    & $strip -o $workExe $ExePath
    Write-Host "Stripped: $((Get-Item $ExePath).Length) -> $((Get-Item $workExe).Length) bytes"
}

$exeBytes = [IO.File]::ReadAllBytes((Resolve-Path $workExe))
if ($exeBytes.Length -gt ($FloppyMax - 1024)) {
    Write-Error "Exe is $($exeBytes.Length) bytes after strip; too large for 1.44M floppy."
}

Remove-Item -Recurse -Force $disk1 -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $disk1 | Out-Null

$readme = @"
AwakeGuard 9x ROFLMAO
Run AWAKEG.EXE (uses MSVCRT20.DLL in \Windows\System).
"@
[IO.File]::WriteAllBytes((Join-Path $disk1 "AWAKEG.EXE"), $exeBytes)
[IO.File]::WriteAllText((Join-Path $disk1 "README.TXT"), $readme, [Text.Encoding]::ASCII)

if ($IncludeMsvcrt) {
    if (-not (Test-Path $MsvcrtPath)) {
        Write-Error "MSVCRT.DLL not found at $MsvcrtPath (see redist\README.md)"
    }
    if ((Get-Item $MsvcrtPath).Length -gt 400000) {
        Write-Error "MSVCRT.DLL looks like a modern Win10+ build; use a VC6/Win9x copy."
    }
    [IO.File]::WriteAllBytes((Join-Path $disk1 "MSVCRT.DLL"), [IO.File]::ReadAllBytes($MsvcrtPath))
    if ((Get-ChildItem $disk1 | Measure-Object -Property Length -Sum).Sum -gt $FloppyMax) {
        Write-Error "AWAKEG.EXE + MSVCRT.DLL exceed 1.44M; use default msvcrt20 build instead."
    }
}

Write-Host "disk1:"; Get-ChildItem $disk1 | Format-Table Name, Length -AutoSize

if (-not $DiskImageJs) {
    foreach ($c in @(
            (Join-Path $PSScriptRoot "..\..\pcjs\tools\diskimage\diskimage.js"),
            (Join-Path $PSScriptRoot "..\pcjs\tools\diskimage\diskimage.js")
        )) {
        if (Test-Path $c) { $DiskImageJs = $c; break }
    }
}

if ($DiskImageJs -and (Test-Path $DiskImageJs) -and (Get-Command node -ErrorAction SilentlyContinue)) {
    Push-Location (Split-Path $DiskImageJs)
    try {
        & node (Split-Path -Leaf $DiskImageJs) --overwrite --target=1440K --dir="$disk1/" --output="$floppyRoot\DISK1.img"
        Write-Host "Wrote DISK1.img"
        & node (Split-Path -Leaf $DiskImageJs) --disk="$floppyRoot\DISK1.img" --list=dir
    }
    finally { Pop-Location }
}
else {
    Write-Host "Staged $disk1 (install Node + pcjs diskimage.js to build DISK1.img)."
}

Remove-Item (Join-Path $floppyRoot "AWAKEG.EXE") -Force -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force (Join-Path $floppyRoot "disk2") -ErrorAction SilentlyContinue
