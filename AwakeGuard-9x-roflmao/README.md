# AwakeGuard 9x ROFLMAO

MinGW museum build of AwakeGuard for **Windows 95 / 98 / ME** and later (32-bit). Same tray idea as the main app, compiled for the bit.

This is **not** the supported production build. It installs to `%AppData%\AwakeGuard-9x-ROFLMAO` (or `%LocalAppData%` when available) so it can coexist with normal AwakeGuard and the XP LOL build.

## What works where

| OS | Screensaver off | Prevent sleep |
|----|-----------------|---------------|
| Windows 95 / 98 / ME | Yes (`SystemParametersInfo`) | No API (checkbox disabled) |
| Windows 2000+ | Yes | Yes (`SetThreadExecutionState`, loaded at runtime) |
| Windows XP+ | Yes | Yes (same binary as 9x ROFLMAO; XP LOL is a separate fork) |

Tray notification balloons require **Windows 2000+**; on 9x you only get the icon and tip text.

This build is **ANSI** (`WinMain`, `Shell_NotifyIconA`, narrow paths) for real **Windows 95** tray compatibility—not a Unicode shell build.

## Features

- System tray (16×16 icons, BMP-in-ICO)
- Screensaver disable while active
- Sleep prevention on Win2000+ (same binary)
- Settings in `settings.json`
- Optional copy-to-AppData + Run key

## Build (local cross-compile)

Any MinGW-w64 i686 GCC works (MSYS2 MinGW32, or Ubuntu's `g++-mingw-w64-i686`): the exe links no C/C++ runtime, so the toolchain's runtime libraries don't matter.

```bash
cd AwakeGuard-9x-roflmao
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686-9x.cmake \
  -DAWAKEGUARD_VERSION=0.1.2
cmake --build build
# -> build/AwakeGuard-9x-ROFLMAO.exe
```

**Release CI** builds on MSYS2 MinGW32 (Windows runner) with `-DAWAKEGUARD_VERSION` from the tag; artifact: `AwakeGuard-win32-x86-9x-roflmao.exe`.

From repo root, ensure icons exist:

```bash
python3 scripts/generate-icons.py
```

## Build (Windows + MSYS2)

Same toolchain as XP LOL (MinGW **i686**):

```powershell
cd AwakeGuard-9x-roflmao
.\build.ps1
# Optional: match a release tag locally
$env:AWAKEGUARD_VERSION = "0.1.2"
.\build.ps1
# Full wipe if cmake gets confused (close anything using build\ first)
.\build.ps1 -Clean
```

Requires [MSYS2](https://www.msys2.org/) with `mingw-w64-i686-gcc`, `cmake`, `make`, `binutils` in the **MinGW 32-bit** environment.

## Deploy to a 9x VM

1. Copy `AwakeGuard-9x-ROFLMAO.exe` to the VM.
2. Run once; use the tray icon (right-click for menu).
3. **Windows 95** needs the tray icon shell (typically **IE 4** / Active Desktop era). Plain 95 without updated shell may not show tray icons. The binary imports only stock Win95 DLLs (`KERNEL32`, `USER32`, `GDI32`, `SHELL32`, `COMCTL32`, `ADVAPI32` for Run-key only—no C runtime, no CryptoAPI, no `SHLWAPI`).
4. Optional: use “Start with Windows” to copy into AppData and register Run.

**No runtime, 386 and up:** the exe is linked with `-nostdlib` and has its own entry point (`src/rt_entry.cpp`), `mem*` functions (`src/rt_mem.c`) and fixed-size strings (`src/str.h`) instead of the C/C++ standard library. MinGW's prebuilt CRT, libgcc and libstdc++ are compiled for Pentium 4 (`LOCK CMPXCHG`, `CMOV`, SSE), which a 386/486 can't execute and Windows 95 can't run on any CPU. Our code is built with `-march=i386`, and CI runs [`scripts/check-9x-exe.py`](../scripts/check-9x-exe.py) to reject any non-386 instruction or non-Win95 DLL import. So: no `std::` in this project, and keep stack frames under 4 KB (no `__chkstk`).

**Floppy / VM:** `.\scripts\prepare-floppy.ps1` stages a 1.44M disk image (the exe fits on one) for real 386+ hardware, 86Box, or the PCjs Windows 95 machine (a Compaq DeskPro 386).

Icons are shared from `../assets/` (run `python scripts/generate-icons.py` at repo root if missing).

## vs AwakeGuard XP LOL

| | **9x ROFLMAO** | **XP LOL** |
|---|-------------|------------|
| Target | `WINVER` 4.0, subsystem 4.0, 80386 | `WINVER` 5.01, subsystem 5.01 |
| Sleep API | Dynamic load (2000+) | Always available |
| AppData | `%AppData%` fallback | `%LocalAppData%` |
| Manifest | No Comctl32 v6 | Comctl32 v6 |
