<#
.SYNOPSIS
    Install AwakeGuard.

.DESCRIPTION
    Picks the best build of AwakeGuard for the current machine and installs it
    to %LOCALAPPDATA%\AwakeGuard.

    Auto-detect logic:
      1. If -Variant is given, install that variant (skips all detection).
      2. Otherwise, if Windows is older than build 17763 (Win 10 1809),
         install the native Win32 build.
      3. Otherwise, if -Choose is given, show the picker.
      4. Otherwise, if the .NET 10 Desktop Runtime is already installed for the
         current architecture, install the framework-dependent WPF build.
      5. Otherwise, show the picker (Win32 / FrameworkDependent / SelfContained).

.PARAMETER Variant
    Force a specific build: Win32, FrameworkDependent, or SelfContained.

.PARAMETER Choose
    Show the interactive picker even when auto-detection would have decided.

.PARAMETER Force
    Overwrite an existing install and stop a running AwakeGuard without prompting.

.PARAMETER NoLaunch
    Don't start AwakeGuard after install.

.EXAMPLE
    irm https://raw.githubusercontent.com/KiwiGeek/AwakeGuard/master/scripts/install.ps1 | iex

.EXAMPLE
    # Pass parameters via the scriptblock form:
    & ([scriptblock]::Create((irm https://raw.githubusercontent.com/KiwiGeek/AwakeGuard/master/scripts/install.ps1))) -Variant Win32 -Force
#>

# Everything below runs inside `& { ... } @args` so that, when this script is
# fed to Invoke-Expression via `irm ... | iex`, the param block lives in its
# own scope. Without this wrapper, any pre-existing $Variant / $Force /
# $Choose / $NoLaunch variable in the caller's shell would either short-circuit
# parameter binding or fail the [ValidateSet] check. `@args` forwards any
# positional/named arguments through when the script is invoked via the
# `& ([scriptblock]::Create((irm ...))) -Foo Bar` form.

& {
    [CmdletBinding()]
    param(
        [ValidateSet('Win32', 'FrameworkDependent', 'SelfContained')]
        [string]$Variant,
        [switch]$Choose,
        [switch]$Force,
        [switch]$NoLaunch
    )

$ErrorActionPreference = 'Stop'
$ProgressPreference    = 'SilentlyContinue'

$RepoOwner  = 'KiwiGeek'
$RepoName   = 'AwakeGuard'
$AppName    = 'AwakeGuard'
$ExeName    = 'AwakeGuard.exe'
$InstallDir = Join-Path $env:LOCALAPPDATA $AppName
$InstallExe = Join-Path $InstallDir $ExeName
$MetaFile   = Join-Path $InstallDir 'install.json'

# Win 10 1809 = build 17763 is the official .NET 10 minimum.
$MinDotNet10Build = 17763

# ---------------------------------------------------------------------------
# Console helpers
# ---------------------------------------------------------------------------

function Write-Step($message) {
    Write-Host ''
    Write-Host '==> ' -ForegroundColor Cyan -NoNewline
    Write-Host $message
}
function Write-Note($message) {
    Write-Host "    $message" -ForegroundColor DarkGray
}
function Write-Ok($message) {
    Write-Host '[ok] ' -ForegroundColor Green -NoNewline
    Write-Host $message
}
function Write-Warn($message) {
    Write-Host '[!]  ' -ForegroundColor Yellow -NoNewline
    Write-Host $message
}
function Fail($message) {
    Write-Host '[x]  ' -ForegroundColor Red -NoNewline
    Write-Host $message
    throw $message
}

# ---------------------------------------------------------------------------
# System probes
# ---------------------------------------------------------------------------

function Get-MachineArchitecture {
    # Two independent probes so a flaky one doesn't kill the install.
    #
    # 1. RuntimeInformation.OSArchitecture: framework-agnostic, returns the
    #    true OS arch even inside a WOW64 PowerShell host. We cast to string
    #    and dispatch on its name to avoid any enum-vs-enum switch subtlety.
    #
    # 2. PROCESSOR_ARCHITECTURE / PROCESSOR_ARCHITEW6432 env vars: ancient,
    #    always populated on Windows, completely independent of .NET state.
    #    PROCESSOR_ARCHITEW6432 is set only when a 32-bit process runs under
    #    WOW64; when present it overrides PROCESSOR_ARCHITECTURE (which would
    #    report x86 in that case).

    $rtName = $null
    try {
        $rt = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture
        if ($null -ne $rt) { $rtName = [string]$rt }
    } catch {}

    switch ($rtName) {
        'X64'   { return 'x64'   }
        'X86'   { return 'x86'   }
        'Arm64' { return 'arm64' }
    }

    $envArch = $env:PROCESSOR_ARCHITEW6432
    if (-not $envArch) { $envArch = $env:PROCESSOR_ARCHITECTURE }

    switch ($envArch) {
        'AMD64' { return 'x64'   }
        'x86'   { return 'x86'   }
        'ARM64' { return 'arm64' }
    }

    Fail ("Could not determine the OS architecture. " +
          "RuntimeInformation=[$rtName] " +
          "PROCESSOR_ARCHITECTURE=[$env:PROCESSOR_ARCHITECTURE] " +
          "PROCESSOR_ARCHITEW6432=[$env:PROCESSOR_ARCHITEW6432]")
}

function Get-WindowsBuildNumber {
    try {
        return [int](Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' -ErrorAction Stop).CurrentBuildNumber
    } catch {
        return 0
    }
}

function Test-WindowsSupportsDotNet10 {
    return (Get-WindowsBuildNumber) -ge $MinDotNet10Build
}

function Get-DotNetInfo {
    # Returns an object with .Available, .Arch, and .Has10Desktop
    # describing the dotnet CLI on PATH.
    $result = [pscustomobject]@{
        Available     = $false
        Arch          = $null
        Has10Desktop  = $false
    }
    if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) {
        return $result
    }
    $result.Available = $true

    try {
        $info = (& dotnet --info 2>$null) | Out-String
    } catch {
        return $result
    }
    if ($info -match '(?im)^\s*Architecture:\s*(\S+)') {
        $a = $matches[1].Trim().ToLowerInvariant()
        if     ($a -eq 'arm64') { $result.Arch = 'arm64' }
        elseif ($a -eq 'x86')   { $result.Arch = 'x86'   }
        else                    { $result.Arch = 'x64'   }
    }

    try {
        $runtimes = & dotnet --list-runtimes 2>$null
        $result.Has10Desktop = [bool]($runtimes | Where-Object { $_ -match '^Microsoft\.WindowsDesktop\.App\s+10\.' })
    } catch {}

    return $result
}

# ---------------------------------------------------------------------------
# Variant selection
# ---------------------------------------------------------------------------

function Show-VariantPicker([string]$arch, [bool]$dotnet10Possible, [bool]$hasDotNet10Runtime) {
    # Build the menu dynamically so we never offer an option that wouldn't run.
    $options = New-Object System.Collections.Generic.List[pscustomobject]
    $options.Add([pscustomobject]@{
        Variant = 'Win32'
        Title   = "Native Win32 ($arch)"
        Detail  = 'Smallest (~200 KB). No dependencies.'
    })
    if ($dotnet10Possible -and $hasDotNet10Runtime) {
        $options.Add([pscustomobject]@{
            Variant = 'FrameworkDependent'
            Title   = ".NET 10 WPF, framework-dependent ($arch)"
            Detail  = 'Modern Fluent UI. Uses the .NET 10 Desktop Runtime already installed.'
        })
    }
    if ($dotnet10Possible) {
        $options.Add([pscustomobject]@{
            Variant = 'SelfContained'
            Title   = ".NET 10 WPF, self-contained ($arch)"
            Detail  = 'Modern Fluent UI. ~100 MB. Bundles the .NET 10 runtime.'
        })
    }

    Write-Host ''
    Write-Host 'Which AwakeGuard build do you want to install?' -ForegroundColor White
    Write-Host ''
    for ($i = 0; $i -lt $options.Count; $i++) {
        $n = $i + 1
        Write-Host ("  [{0}] {1}" -f $n, $options[$i].Title) -ForegroundColor White
        Write-Host ("        {0}" -f $options[$i].Detail)     -ForegroundColor DarkGray
    }
    Write-Host ''

    $validKeys = (1..$options.Count) -join ', '
    while ($true) {
        $reply = (Read-Host "Pick a number ($validKeys)").Trim()
        $idx = 0
        if ([int]::TryParse($reply, [ref]$idx) -and $idx -ge 1 -and $idx -le $options.Count) {
            return $options[$idx - 1].Variant
        }
        Write-Warn "Please type one of: $validKeys."
    }
}

function Resolve-Variant([string]$arch) {
    if ($Variant) {
        Write-Note "Variant forced to $Variant."
        return $Variant
    }

    if (-not (Test-WindowsSupportsDotNet10)) {
        $build = Get-WindowsBuildNumber
        Write-Note "Windows build $build is older than $($MinDotNet10Build) (Win 10 1809). Installing the Win32 build."
        return 'Win32'
    }

    $info = Get-DotNetInfo
    $hasMatchingRuntime = $info.Available -and $info.Has10Desktop -and $info.Arch -eq $arch

    if ($Choose) {
        return Show-VariantPicker -arch $arch -dotnet10Possible $true -hasDotNet10Runtime $hasMatchingRuntime
    }

    if ($hasMatchingRuntime) {
        Write-Note ".NET 10 Desktop Runtime ($arch) detected. Installing the framework-dependent WPF build."
        return 'FrameworkDependent'
    }

    if ($info.Available -and $info.Has10Desktop -and $info.Arch -and $info.Arch -ne $arch) {
        Write-Note ".NET 10 Desktop Runtime is installed, but for $($info.Arch) instead of $arch. Asking you what to install:"
    } else {
        Write-Note '.NET 10 Desktop Runtime not detected. Asking you what to install:'
    }
    return Show-VariantPicker -arch $arch -dotnet10Possible $true -hasDotNet10Runtime $false
}

# ---------------------------------------------------------------------------
# GitHub release lookup
# ---------------------------------------------------------------------------

function Get-LatestReleaseAsset([string]$variant, [string]$arch) {
    Write-Step 'Looking up the latest AwakeGuard release...'
    $apiUrl  = "https://api.github.com/repos/$($RepoOwner)/$($RepoName)/releases/latest"
    $headers = @{ 'User-Agent' = 'AwakeGuard-installer' }
    if ($env:GITHUB_TOKEN) { $headers['Authorization'] = "Bearer $env:GITHUB_TOKEN" }

    try {
        $release = Invoke-RestMethod -Uri $apiUrl -Headers $headers
    } catch {
        Fail "Failed to query GitHub: $($_.Exception.Message)"
    }

    Write-Note "Latest release: $($release.tag_name)"

    $assetName = switch ($variant) {
        'Win32'              { "AwakeGuard-win32-$arch.exe" }
        'FrameworkDependent' { "AwakeGuard-wpf-framework-dependent-win-$arch.zip" }
        'SelfContained'      { "AwakeGuard-wpf-self-contained-win-$arch.zip" }
    }

    $asset = $release.assets | Where-Object { $_.name -eq $assetName } | Select-Object -First 1
    if (-not $asset) {
        Fail "Release $($release.tag_name) is missing the expected asset '$assetName'."
    }

    return [pscustomobject]@{
        Tag  = $release.tag_name
        Name = $asset.name
        Url  = $asset.browser_download_url
        Size = [long]$asset.size
    }
}

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------

function Stop-RunningAwakeGuard {
    $procs = Get-Process -Name 'AwakeGuard' -ErrorAction SilentlyContinue
    if (-not $procs) { return }

    if (-not $Force) {
        $reply = Read-Host 'AwakeGuard is currently running. Stop it and continue? [Y/n]'
        if ($reply -and $reply -notmatch '^[Yy]') { Fail 'Install aborted by user.' }
    }

    Write-Note 'Stopping running AwakeGuard...'
    foreach ($p in $procs) {
        try { $p.Kill() } catch {}
    }
    Start-Sleep -Milliseconds 500
}

function Clear-OldInstall {
    if (-not (Test-Path $InstallDir)) { return }
    Get-ChildItem -Path $InstallDir -File -Force -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -in '.exe', '.dll', '.pdb' -or $_.Name -eq 'install.json' } |
        Remove-Item -Force -ErrorAction SilentlyContinue
    # NB: settings.json (managed by the app) is intentionally preserved.
}

function Install-Asset([pscustomobject]$asset, [string]$variant, [string]$arch) {
    $tempDir = Join-Path $env:TEMP "AwakeGuard-install-$([Guid]::NewGuid())"
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

    try {
        $downloadPath = Join-Path $tempDir $asset.Name
        $sizeMb = [math]::Round($asset.Size / 1MB, 1)
        Write-Step "Downloading $($asset.Name) ($sizeMb MB)..."
        Invoke-WebRequest -Uri $asset.Url -OutFile $downloadPath -UseBasicParsing -Headers @{ 'User-Agent' = 'AwakeGuard-installer' }

        if ((Get-Item $downloadPath).Length -lt 1024) {
            Fail "Downloaded file is suspiciously small ($((Get-Item $downloadPath).Length) bytes). Aborting."
        }

        Stop-RunningAwakeGuard

        if (-not (Test-Path $InstallDir)) {
            New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
        }
        Clear-OldInstall

        Write-Step "Installing to $($InstallExe)"

        if ($asset.Name -like '*.zip') {
            $extractDir = Join-Path $tempDir 'extracted'
            Expand-Archive -Path $downloadPath -DestinationPath $extractDir -Force
            $exe = Get-ChildItem -Path $extractDir -Recurse -Filter $ExeName | Select-Object -First 1
            if (-not $exe) {
                Fail "Couldn't find $($ExeName) inside $($asset.Name)."
            }
            Copy-Item -Path (Join-Path $extractDir '*') -Destination $InstallDir -Recurse -Force
        } else {
            Copy-Item -Path $downloadPath -Destination $InstallExe -Force
        }

        $meta = [pscustomobject]@{
            Variant     = $variant
            Tag         = $asset.Tag
            Asset       = $asset.Name
            Arch        = $arch
            InstalledAt = (Get-Date).ToString('o')
        }
        $meta | ConvertTo-Json | Set-Content -Path $MetaFile -Encoding UTF8

        Write-Ok "Installed AwakeGuard $($asset.Tag) ($variant, $arch)."
    } finally {
        Remove-Item -Recurse -Force -Path $tempDir -ErrorAction SilentlyContinue
    }
}

# ---------------------------------------------------------------------------
# Uninstall registration (Apps & Features / Programs and Features)
# ---------------------------------------------------------------------------

# The uninstall.ps1 script lives next to the exe and is referenced by the
# UninstallString registry value. It is intentionally self-contained so it can
# be invoked by Windows without our installer module.
$UninstallScriptBody = @'
[CmdletBinding()]
param([switch]$Silent)

$ErrorActionPreference = 'SilentlyContinue'

$AppName      = 'AwakeGuard'
$InstallDir   = Join-Path $env:LOCALAPPDATA $AppName
$ShortcutPath = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\AwakeGuard.lnk'
$RunKeyPath   = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$UninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\AwakeGuard'

if (-not $Silent) {
    try {
        Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop
        $reply = [System.Windows.Forms.MessageBox]::Show(
            "Uninstall AwakeGuard?`r`n`r`nThis removes the application and its settings.",
            'Uninstall AwakeGuard',
            [System.Windows.Forms.MessageBoxButtons]::YesNo,
            [System.Windows.Forms.MessageBoxIcon]::Question)
        if ($reply -ne [System.Windows.Forms.DialogResult]::Yes) { exit 1 }
    } catch {
        # If WinForms is unavailable, just proceed silently.
    }
}

Get-Process -Name 'AwakeGuard' -ErrorAction SilentlyContinue | ForEach-Object {
    try { $_.Kill() } catch {}
}
Start-Sleep -Milliseconds 500

Remove-ItemProperty -Path $RunKeyPath -Name $AppName -ErrorAction SilentlyContinue
Remove-Item -Path $ShortcutPath -Force -ErrorAction SilentlyContinue
Remove-Item -Path $UninstallKey -Recurse -Force -ErrorAction SilentlyContinue

# We are currently executing from inside $InstallDir, so we cannot delete it
# from here. Spawn a detached .bat file that waits a couple of seconds for
# this PowerShell process to exit, then removes the whole tree and deletes
# itself.
$batPath = Join-Path $env:TEMP ('awakeguard-cleanup-' + [Guid]::NewGuid().ToString('N') + '.bat')
$batBody = @"
@echo off
ping 127.0.0.1 -n 3 >nul
taskkill /IM AwakeGuard.exe /F >nul 2>&1
rd /s /q "$InstallDir"
del "%~f0"
"@
Set-Content -Path $batPath -Value $batBody -Encoding ASCII
Start-Process -FilePath $batPath -WindowStyle Hidden

if (-not $Silent) {
    try {
        Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop
        [void][System.Windows.Forms.MessageBox]::Show(
            'AwakeGuard has been uninstalled.',
            'AwakeGuard',
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Information)
    } catch {}
}
'@

function Write-UninstallScript {
    $path = Join-Path $InstallDir 'uninstall.ps1'
    Set-Content -Path $path -Value $UninstallScriptBody -Encoding UTF8
    Write-Note "Wrote $path"
}

function Register-Uninstall([pscustomobject]$asset, [string]$variant, [string]$arch) {
    $regPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\AwakeGuard'

    # Recreate from scratch so stale values from previous installs don't linger.
    Remove-Item -Path $regPath -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -Path $regPath -Force | Out-Null

    $uninstallScriptPath = Join-Path $InstallDir 'uninstall.ps1'
    $uninstallCmd      = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$uninstallScriptPath`""
    $quietUninstallCmd = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$uninstallScriptPath`" -Silent"

    $sizeBytes = (Get-ChildItem -Path $InstallDir -Recurse -File -ErrorAction SilentlyContinue |
        Measure-Object -Property Length -Sum).Sum
    $sizeKb = if ($sizeBytes) { [int]($sizeBytes / 1024) } else { 0 }

    $displayName = switch ($variant) {
        'Win32'              { "AwakeGuard ($arch, native)" }
        'FrameworkDependent' { "AwakeGuard ($arch, WPF .NET 10)" }
        'SelfContained'      { "AwakeGuard ($arch, WPF .NET 10 self-contained)" }
        default              { "AwakeGuard ($arch)" }
    }

    $stringProps = @{
        DisplayName          = $displayName
        DisplayVersion       = $asset.Tag
        Publisher            = 'KiwiGeek'
        DisplayIcon          = $InstallExe
        InstallLocation      = $InstallDir
        UninstallString      = $uninstallCmd
        QuietUninstallString = $quietUninstallCmd
        InstallDate          = (Get-Date).ToString('yyyyMMdd')
        URLInfoAbout         = "https://github.com/$($RepoOwner)/$($RepoName)"
        URLUpdateInfo        = "https://github.com/$($RepoOwner)/$($RepoName)/releases"
        Comments             = "$variant build for $arch"
    }
    foreach ($k in $stringProps.Keys) {
        New-ItemProperty -Path $regPath -Name $k -Value $stringProps[$k] -PropertyType String -Force | Out-Null
    }

    $dwordProps = @{
        NoModify      = 1
        NoRepair      = 1
        EstimatedSize = $sizeKb
    }
    foreach ($k in $dwordProps.Keys) {
        New-ItemProperty -Path $regPath -Name $k -Value $dwordProps[$k] -PropertyType DWord -Force | Out-Null
    }

    Write-Note 'Registered under Apps & Features (per-user).'
}

# ---------------------------------------------------------------------------
# Start Menu shortcut
# ---------------------------------------------------------------------------

function New-StartMenuShortcut {
    $startMenu = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs'
    if (-not (Test-Path $startMenu)) {
        New-Item -ItemType Directory -Path $startMenu -Force | Out-Null
    }
    $lnkPath = Join-Path $startMenu 'AwakeGuard.lnk'

    try {
        $shell = New-Object -ComObject WScript.Shell
        $shortcut = $shell.CreateShortcut($lnkPath)
        $shortcut.TargetPath       = $InstallExe
        $shortcut.WorkingDirectory = $InstallDir
        $shortcut.IconLocation     = "$($InstallExe),0"
        $shortcut.Description      = 'Keep your PC awake and prevent automatic lock or sleep.'
        $shortcut.Save()
        Write-Note "Start Menu shortcut: $lnkPath"
    } catch {
        Write-Warn "Could not create Start Menu shortcut: $($_.Exception.Message)"
    }
}

# ---------------------------------------------------------------------------
# Launch
# ---------------------------------------------------------------------------

function Start-Installed {
    Write-Step "Launching $($ExeName)..."
    Start-Process -FilePath $InstallExe -WorkingDirectory $InstallDir

    # Give the tray icon a moment to appear before the MessageBox steals focus.
    Start-Sleep -Milliseconds 800
    Show-TrayHint
}

function Show-TrayHint {
    $title = 'AwakeGuard is running'

    # Each entry is one paragraph. The MessageBox does its own word-wrap
    # within the dialog width, so we must NOT insert mid-paragraph line
    # breaks here -- those would render as forced wraps in awkward places.
    $paragraphs = @(
        'AwakeGuard runs in the system tray, not as a window.'
        'Look in the notification area at the bottom-right of the taskbar for a small circle icon. It is gray when off and green when active.'
        'If you do not see it, click the small up-arrow (^) next to the clock to reveal hidden icons. You can drag it onto the always-visible row to pin it.'
        'Left-click the icon to toggle. Right-click for the menu. Double-click to open the settings window.'
    )
    $body = $paragraphs -join "`r`n`r`n"

    try {
        Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop
        [void][System.Windows.Forms.MessageBox]::Show(
            $body,
            $title,
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Information)
    } catch {
        # Fallback for hosts where Windows Forms isn't available
        # (e.g. PowerShell Core without -STA): print to console instead.
        Write-Host ''
        Write-Host "  $title" -ForegroundColor Cyan
        Write-Host ''
        foreach ($p in $paragraphs) {
            Write-Host "  $p"
            Write-Host ''
        }
    }
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

Write-Host ''
Write-Host '  AwakeGuard installer' -ForegroundColor Cyan
Write-Host "  https://github.com/$($RepoOwner)/$($RepoName)" -ForegroundColor DarkGray

$arch = Get-MachineArchitecture
Write-Note "Detected architecture: $arch"

$selectedVariant = Resolve-Variant -arch $arch
$asset = Get-LatestReleaseAsset -variant $selectedVariant -arch $arch
Install-Asset -asset $asset -variant $selectedVariant -arch $arch

Write-UninstallScript
Register-Uninstall -asset $asset -variant $selectedVariant -arch $arch

New-StartMenuShortcut

if (-not $NoLaunch) {
    Start-Installed
}

Write-Host ''
Write-Ok 'All done.'
Write-Host ''

} @args
