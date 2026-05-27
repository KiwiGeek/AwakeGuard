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

## Build artifacts produced by CI

Pushing to the `release` branch triggers
[`.github/workflows/release.yml`](.github/workflows/release.yml), which
produces three artifacts:

1. **`AwakeGuard-win32-x64.exe`** — native Win32 build, no dependencies.
2. **`AwakeGuard-wpf-framework-dependent-win-x64.zip`** — .NET 10 WPF build that
   requires the .NET 10 Desktop Runtime to be installed on the target machine.
   Small download.
3. **`AwakeGuard-wpf-self-contained-win-x64.zip`** — .NET 10 WPF build with the
   runtime bundled in. Larger download, no .NET install required.

## Building locally

### WPF version (.NET 10)

Requires the [.NET 10 SDK](https://dotnet.microsoft.com/download).

```powershell
cd AwakeGuard
dotnet run -c Release

# Framework-dependent publish
dotnet publish -c Release -r win-x64 --self-contained false -o publish/framework-dependent

# Self-contained single-file publish
dotnet publish -c Release -r win-x64 --self-contained true `
    /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true `
    -o publish/self-contained
```

### Native Win32 version

Requires Visual Studio 2022 (or the Build Tools) with the **Desktop
development with C++** workload.

```powershell
cd AwakeGuard-win32
.\build.bat
# → bin\Release\AwakeGuard.exe
```

Or, if you have CMake:

```powershell
cd AwakeGuard-win32
cmake -S . -B build -A x64
cmake --build build --config Release
```

See [`AwakeGuard-win32/README.md`](AwakeGuard-win32/README.md) for details.

## Repository layout

```
AwakeGuard/                  WPF / .NET 10 implementation
AwakeGuard-win32/            Native Win32 / C++17 implementation
.github/workflows/           CI pipelines
  release.yml                Builds all three artifacts on push to `release`
.gitignore                   Combined VS + CMake ignores
.gitattributes               Line-ending + diff hints
README.md                    You are here
```

## License

TBD.
