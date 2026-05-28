# AwakeGuard XP LOL

MinGW museum build of AwakeGuard for **Windows XP SP2+** (32-bit). Same idea as the native Win32 app, but compiled for the bit.

This is **not** the supported production build. It installs to `%LocalAppData%\AwakeGuard-XP-LOL` so it can coexist with normal AwakeGuard.

## What works on XP

- System tray (16x16 icons)
- `SetThreadExecutionState` / `ES_SYSTEM_REQUIRED` (prevent sleep)
- Screensaver disable via `SystemParametersInfo` when "screensaver" protection is on
- Settings, durations, startup Run key, copy-to-AppData flow

## What does not

- **Display lock** prevention (Vista+ API) — checkbox only affects screensaver on XP
- SysLink About links (replaced with buttons)
- DWM / modern chrome / Segoe UI

## Build on Linux / GitHub Actions

```bash
sudo apt-get install -y g++-mingw-w64-i686-w64-mingw32
cmake -S AwakeGuard-xp-lol -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686-xp.cmake
cmake --build build --config Release
# -> build/AwakeGuard-XP-LOL.exe
```

## Build on Windows (MSYS2 MinGW 32-bit)

```powershell
pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-cmake make
$env:PATH = "C:\msys64\mingw32\bin;" + $env:PATH
cmake -S AwakeGuard-xp-lol -B build -G "MinGW Makefiles" `
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686-xp.cmake `
  -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc `
  -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++
cmake --build build
```

Or run `.\build.ps1` from this folder after MSYS2 i686 gcc is on PATH.

## Test in your XP VM

1. Copy `AwakeGuard-XP-LOL.exe` to the VM (and `AwakeGuard.ico` is embedded).
2. Run it. Look for the shield in the tray notification area.
3. Turn on protection; sleep should be inhibited while active.
4. Optional: copy to `%LocalAppData%\AwakeGuard-XP-LOL` via the startup checkbox flow.

Icons are shared from `../assets/` (run `python scripts/generate-icons.py` at repo root if missing).
