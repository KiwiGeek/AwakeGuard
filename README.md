# AwakeGuard

A tiny Windows system-tray app that temporarily keeps your PC awake and
prevents the screensaver / display lock from kicking in. Toggle it from the
tray, pick a duration (30 min, 1 h, 2 h, 4 h, 8 h, rest of today, or until you
turn it off), and get on with what you were doing.

Under the hood it just calls
[`SetThreadExecutionState`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate)
with `ES_SYSTEM_REQUIRED` (sleep) and `ES_DISPLAY_REQUIRED` (display). No power
plan changes, no services, no drivers.

This repository contains **four** codebases: two supported implementations (WPF
and native Win32), plus two MinGW museum ports (XP LOL and 9x ROFLMAO). Release
CI on the `release` branch ships **eleven** binaries (Win32 and WPF × three
architectures, plus the XP LOL and 9x ROFLMAO i686 museum builds).

## Flavors

| Flavor                              | Folder                | Tech                                  | Why you'd pick it                                                                                              |
| ----------------------------------- | --------------------- | ------------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| **AwakeGuard (Fluent / WPF)**       | [`AwakeGuard/`](AwakeGuard/) | .NET 10, WPF, [WPF-UI](https://github.com/lepoco/wpfui), Hardcodet.NotifyIcon | The "pretty" one. Modern Fluent / Mica UI. Requires (or bundles) the .NET 10 runtime. |
| **AwakeGuard (Native Win32)**       | [`AwakeGuard-win32/`](AwakeGuard-win32/) | C++17, Win32 API only, no third‑party libs | The "tiny" one. Single ~200 KB exe. No runtime needed beyond Windows itself.        |
| **AwakeGuard XP LOL** (unserious)   | [`AwakeGuard-xp-lol/`](AwakeGuard-xp-lol/) | MinGW i686, Win32, targets Windows XP | Museum build for VMs and nostalgia. Separate AppData folder. See its README.       |
| **AwakeGuard 9x ROFLMAO** (unhinged)  | [`AwakeGuard-9x-roflmao/`](AwakeGuard-9x-roflmao/) | MinGW i686, Win32, targets 95/98/ME+ | Even older museum build; screensaver on 9x, sleep on Win2000+. See its README.     |

Both main versions are feature-equivalent (same tray menu, same settings, same
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
   `%LOCALAPPDATA%\AwakeGuard`, creates a Start Menu shortcut, registers AwakeGuard
   under **Apps & Features** (per-user), and launches it.

To remove it later: **Settings → Apps → Installed apps → AwakeGuard → Uninstall**,
or just run `%LOCALAPPDATA%\AwakeGuard\uninstall.ps1`.

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

## Releasing

See **[RELEASING.md](RELEASING.md)** for the full deploy checklist (tag →
fast-forward `release` → CI → verify GitHub Release). Summary: tag the commit
on `master` as `vX.Y.Z`, push the tag, then `git merge --ff-only master` on
`release` and push `release`.

## Build artifacts produced by CI

Pushing to the `release` branch (with a version tag at `HEAD`) triggers
[`.github/workflows/release.yml`](.github/workflows/release.yml), which
produces **eleven** release binaries plus `LICENSE` (three mainstream variants × three architectures, plus two MinGW museum builds):

| Variant                              | x64                                                       | x86                                                       | ARM64                                                       |
| ------------------------------------ | --------------------------------------------------------- | --------------------------------------------------------- | ----------------------------------------------------------- |
| Native Win32                         | `AwakeGuard-win32-x64.exe`                                | `AwakeGuard-win32-x86.exe`                                | `AwakeGuard-win32-arm64.exe`                                |
| Win32 XP LOL (MinGW, Windows XP)     | —                                                         | `AwakeGuard-win32-x86-xp-lol.exe`                         | —                                                           |
| Win32 9x ROFLMAO (MinGW, Windows 95 / 98 / ME) | —                                                 | `AwakeGuard-win32-x86-9x-roflmao.exe`                     | —                                                           |
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

### XP LOL (MinGW, Windows XP)

Requires MSYS2 **i686** MinGW or Linux `g++-mingw-w64-i686-w64-mingw32`. See [`AwakeGuard-xp-lol/README.md`](AwakeGuard-xp-lol/README.md).

```powershell
cd AwakeGuard-xp-lol
.\build.ps1
# -> build\AwakeGuard-XP-LOL.exe  (copy to your XP VM)
```

CI builds this artifact as part of the [Release workflow](.github/workflows/release.yml) when you ship from the `release` branch.

### 9x ROFLMAO (MinGW, Windows 95 / 98 / ME)

ANSI Win32 museum build for real Windows 9x tray shells (and the same `.exe` still runs on newer Windows). See [`AwakeGuard-9x-roflmao/README.md`](AwakeGuard-9x-roflmao/README.md) for the OS capability matrix.

Requires **MSYS2 MinGW i686** (GCC 14+ for `-mcrtdll=msvcrt20`; Ubuntu apt `gcc-mingw-w64-i686` is too old). From the repo root, generate shared icons once if needed:

```powershell
python scripts/generate-icons.py
```

**Windows (MSYS2):**

```powershell
cd AwakeGuard-9x-roflmao
.\build.ps1
# -> build\AwakeGuard-9x-ROFLMAO.exe  (copy to your 95/98/ME VM)
```

`.\build.ps1 -InstallMsys2` can install MSYS2 via winget; use `.\build.ps1 -Clean` if a previous build folder is locked. Install packages in the **MSYS2 MinGW 32-bit** shell: `mingw-w64-i686-gcc`, `mingw-w64-i686-cmake`, `mingw-w64-i686-make`, `mingw-w64-i686-binutils`.

**Linux (cross-compile):** only with MinGW-w64 **GCC 14+** that supports `-mcrtdll=msvcrt20` (stock Ubuntu Noble `gcc-mingw-w64-i686` is GCC 13 and will not work). Release CI builds this target on **MSYS2 MinGW32** (Windows runner).

```bash
python3 scripts/generate-icons.py
cd AwakeGuard-9x-roflmao
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686-9x.cmake
cmake --build build
# -> build/AwakeGuard-9x-ROFLMAO.exe
```

CI builds this artifact as **`AwakeGuard-win32-x86-9x-roflmao.exe`** on the [Release workflow](.github/workflows/release.yml) when you ship from the `release` branch (same tag-driven `AWAKEGUARD_VERSION` as the other builds).

## Repository layout

```
AwakeGuard/                  WPF / .NET 10 implementation
AwakeGuard-win32/            Native Win32 / C++17 implementation
AwakeGuard-xp-lol/           MinGW museum build for Windows XP
AwakeGuard-9x-roflmao/       MinGW museum build for Windows 9x (95/98/ME+)
scripts/
  install.ps1                The irm | iex one-liner installer
RELEASING.md                 Deploy checklist (read this when shipping)
.github/workflows/
  release.yml                Builds all 11 artifacts on push to `release`
.gitignore                   Combined VS + CMake ignores
.gitattributes               Line-ending + diff hints
README.md                    You are here
```

## License

AwakeGuard is licensed under the **GNU General Public License v3.0 or later**
(GPL-3.0-or-later). The full license text is in [`LICENSE`](LICENSE).

In short: you are free to use, study, modify, and redistribute this software,
provided that any distributed derivative works are released under the same
license and ship with their corresponding source code. It comes with **no
warranty** to the extent permitted by law.

```
AwakeGuard - keep your PC awake on your terms.
Copyright (C) 2026 Joshua Penman

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
