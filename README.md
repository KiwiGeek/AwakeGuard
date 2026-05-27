# AwakeGuard

A tiny Windows system-tray app that temporarily keeps your PC awake and
prevents the screensaver / display lock from kicking in. Toggle it from the
tray, pick a duration (30 min, 1 h, 2 h, 4 h, 8 h, rest of today, or until you
turn it off), and get on with what you were doing.

Under the hood it just calls
[`SetThreadExecutionState`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate)
with `ES_SYSTEM_REQUIRED` (sleep) and `ES_DISPLAY_REQUIRED` (display). No power
plan changes, no services, no drivers.

This repository contains **two** implementations of the app plus a CI pipeline
that builds three distributable binaries.

## The two flavors

| Flavor                              | Folder                | Tech                                  | Why you'd pick it                                                                                              |
| ----------------------------------- | --------------------- | ------------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| **AwakeGuard (Fluent / WPF)**       | [`AwakeGuard/`](AwakeGuard/) | .NET 10, WPF, [WPF-UI](https://github.com/lepoco/wpfui), Hardcodet.NotifyIcon | The "pretty" one. Modern Fluent / Mica UI. Requires (or bundles) the .NET 10 runtime. |
| **AwakeGuard (Native Win32)**       | [`AwakeGuard-win32/`](AwakeGuard-win32/) | C++17, Win32 API only, no third‑party libs | The "tiny" one. Single ~200 KB exe. No runtime needed beyond Windows itself.        |

Both versions are feature-equivalent (same tray menu, same settings, same
duration presets, same "copy to `%LocalAppData%\AwakeGuard`" install flow).

## Install (one-liner)

Open PowerShell and run:

```powershell
irm https://raw.githubusercontent.com/KiwiGeek/AwakeGuard/master/scripts/install.ps1 | iex
```

The installer:

1. Detects your architecture (`x64`, `x86`, or `ARM64`) and Windows version.
2. If Windows is older than 10 1809 (build 17763), it installs the **native
   Win32** build (since .NET 10 isn't supported there).
3. Otherwise, if the **.NET 10 Desktop Runtime** is already installed for your
   arch, it installs the **WPF framework-dependent** build (small download).
4. Otherwise, it asks you which build you want:
   - Native Win32 (tiny, no dependencies)
   - WPF framework-dependent (small, needs .NET 10 Desktop Runtime)
   - WPF self-contained (~100 MB, bundles the runtime)
5. Downloads the right asset from the latest GitHub Release, installs it to
   `%LOCALAPPDATA%\AwakeGuard`, creates a Start Menu shortcut, and launches it.

### Optional flags

To pass flags through `irm | iex` you need the scriptblock form:

```powershell
& ([scriptblock]::Create((irm https://raw.githubusercontent.com/KiwiGeek/AwakeGuard/master/scripts/install.ps1))) -Variant Win32
```

| Flag                                           | What it does                                                              |
| ---------------------------------------------- | ------------------------------------------------------------------------- |
| `-Variant Win32\|FrameworkDependent\|SelfContained` | Skip auto-detect; install exactly this build.                          |
| `-Choose`                                      | Force the picker even when auto-detect would have chosen.                 |
| `-Force`                                       | Don't prompt when overwriting an existing install or killing a running app. |
| `-NoLaunch`                                    | Install but don't start AwakeGuard.                                       |

## Build artifacts produced by CI

Pushing to the `release` branch triggers
[`.github/workflows/release.yml`](.github/workflows/release.yml), which
produces **nine** artifacts (three variants × three architectures):

| Variant                              | x64                                                       | x86                                                       | ARM64                                                       |
| ------------------------------------ | --------------------------------------------------------- | --------------------------------------------------------- | ----------------------------------------------------------- |
| Native Win32                         | `AwakeGuard-win32-x64.exe`                                | `AwakeGuard-win32-x86.exe`                                | `AwakeGuard-win32-arm64.exe`                                |
| WPF, framework-dependent (needs .NET 10) | `AwakeGuard-wpf-framework-dependent-win-x64.zip`      | `AwakeGuard-wpf-framework-dependent-win-x86.zip`      | `AwakeGuard-wpf-framework-dependent-win-arm64.zip`      |
| WPF, self-contained (bundles runtime) | `AwakeGuard-wpf-self-contained-win-x64.zip`              | `AwakeGuard-wpf-self-contained-win-x86.zip`              | `AwakeGuard-wpf-self-contained-win-arm64.zip`              |

## Building locally

### WPF version (.NET 10)

Requires the [.NET 10 SDK](https://dotnet.microsoft.com/download).

```powershell
cd AwakeGuard
dotnet run -c Release

# Framework-dependent publish (substitute win-x86 or win-arm64 as needed)
dotnet publish -c Release -r win-x64 --self-contained false `
    -p:PublishSingleFile=true -o publish/framework-dependent

# Self-contained single-file publish
dotnet publish -c Release -r win-x64 --self-contained true `
    -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true `
    -p:EnableCompressionInSingleFile=true -o publish/self-contained
```

### Native Win32 version

Requires Visual Studio 2022 (or the Build Tools) with the **Desktop
development with C++** workload.

```powershell
cd AwakeGuard-win32
.\build.bat            # defaults to x64
.\build.bat Win32      # 32-bit
.\build.bat ARM64      # ARM64
# → bin\<platform>\Release\AwakeGuard.exe
```

Or, if you have CMake:

```powershell
cd AwakeGuard-win32
cmake -S . -B build -A x64    # or -A Win32 / -A ARM64
cmake --build build --config Release
```

See [`AwakeGuard-win32/README.md`](AwakeGuard-win32/README.md) for details.

## Repository layout

```
AwakeGuard/                  WPF / .NET 10 implementation
AwakeGuard-win32/            Native Win32 / C++17 implementation
scripts/
  install.ps1                The irm | iex one-liner installer
.github/workflows/
  release.yml                Builds all 9 artifacts on push to `release`
.gitignore                   Combined VS + CMake ignores
.gitattributes               Line-ending + diff hints
README.md                    You are here
```

## License

TBD.
