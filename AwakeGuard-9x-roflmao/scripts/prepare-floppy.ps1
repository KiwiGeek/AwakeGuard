#Requires -Version 5.1
<#
.SYNOPSIS
  Stage a 1.44M floppy image for Win95 VMs / PCjs (AwakeGuard 9x ROFLMAO).

.DESCRIPTION
  The ROFLMAO exe links no C runtime and is stripped at link time, so the floppy
  holds just AWAKEG.EXE and a README. Builds DISK1.img with PCjs diskimage.js
  when Node and a pcjs checkout are available.
#>
param(
    [string]$ExePath = (Join-Path $PSScriptRoot "..\build\AwakeGuard-9x-ROFLMAO.exe"),
    [string]$OutDir = (Join-Path $PSScriptRoot "..\floppy"),
    [string]$DiskImageJs
)

$ErrorActionPreference = "Stop"
$FloppyMax = 1474560
$floppyRoot = (Resolve-Path (New-Item -ItemType Directory -Force -Path $OutDir)).Path
$disk1 = Join-Path $floppyRoot "disk1"

if (-not (Test-Path $ExePath)) {
    Write-Error "Build the exe first: $ExePath"
}

$exeBytes = [IO.File]::ReadAllBytes((Resolve-Path $ExePath))
if ($exeBytes.Length -gt ($FloppyMax - 1024)) {
    Write-Error "Exe is $($exeBytes.Length) bytes; too large for a 1.44M floppy."
}

Remove-Item -Recurse -Force $disk1 -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $disk1 | Out-Null

$readme = @"
AwakeGuard 9x ROFLMAO
Copy AWAKEG.EXE to the hard disk and run it. Needs Windows 95 or later, 386 or better.
"@
[IO.File]::WriteAllBytes((Join-Path $disk1 "AWAKEG.EXE"), $exeBytes)
[IO.File]::WriteAllText((Join-Path $disk1 "README.TXT"), $readme, [Text.Encoding]::ASCII)

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
