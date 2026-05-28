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

`pacman` does **not** work in PowerShell — only inside an MSYS2 shell.

**One-time setup**

```powershell
# From PowerShell (installs MSYS2)
winget install -e --id MSYS2.MSYS2
# Or: .\build.ps1 -InstallMsys2
```

Then open the Start Menu app **“MSYS2 MinGW 32-bit”** (important — not “MSYS2 MSYS”) and run:

```bash
pacman -Syu --noconfirm
pacman -S --needed --noconfirm mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-make mingw-w64-i686-binutils
```

**Build** (back in PowerShell):

```powershell
cd AwakeGuard-xp-lol
.\build.ps1
# Optional: match a release tag locally
$env:AWAKEGUARD_VERSION = "0.1.2"
.\build.ps1
# Full wipe if cmake gets confused (close anything using build\ first)
.\build.ps1 -Clean
```

`build.ps1` prepends `C:\msys64\mingw32\bin` when MSYS2 is installed there.

## Test in your XP VM

1. Copy `AwakeGuard-XP-LOL.exe` to the VM (and `AwakeGuard.ico` is embedded).
2. Run it. Look for the shield in the tray notification area.
3. Turn on protection; sleep should be inhibited while active.
4. Optional: copy to `%LocalAppData%\AwakeGuard-XP-LOL` via the startup checkbox flow.

Icons are shared from `../assets/` (run `python scripts/generate-icons.py` at repo root if missing).
