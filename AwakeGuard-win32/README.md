# AwakeGuard-win32

Native Win32 reimplementation of AwakeGuard with no third-party dependencies — only the Windows API (plus C++17 standard library).

## Build (Visual Studio — no CMake required)

### Option A: Open in Visual Studio

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) with the **Desktop development with C++** workload.
2. Open `AwakeGuard.sln`.
3. Set configuration to **Release | x64**.
4. Build → Build Solution.

Output: `bin\Release\AwakeGuard.exe`

### Option B: Command line

Double-click `build.bat`, or from a normal Command Prompt:

```cmd
cd C:\Users\joshua\AwakeGuard-win32
build.bat
```

### Prerequisites

Install **Visual Studio 2022 or later** with the **Desktop development with C++** workload (includes MSBuild and the MSVC compiler). No CMake required.

If build fails with "Platform Toolset cannot be found", open `AwakeGuard.sln` in Visual Studio — it will prompt you to install or retarget the C++ tools.

If you install CMake later:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Behavior

Matches the WPF app:

- System tray icon (green when active, gray when off)
- Left-click toggle, double-click settings, context menu with durations
- Settings window: protection options, start with Windows, duration, turn on/off
- `SetThreadExecutionState` for sleep/display lock prevention
- Copy-to-AppData flow with `--cleanup-source` and delete-original prompt
- Registry Run key under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`
- Settings in `%LocalAppData%\AwakeGuard\settings.json`

## Project layout

```
AwakeGuard-win32/
  CMakeLists.txt
  app.manifest          # Common Controls v6
  src/
    main.cpp            # Entry point
    app.cpp             # App lifecycle
    session.cpp         # Awake session + timers
    settings_wnd.cpp    # Settings window UI
    tray.cpp            # Shell notification area
    startup.cpp         # Run key + copy flow
    cleanup.cpp         # Post-copy source cleanup
    paths.cpp           # AppData / Desktop detection
    settings_store.cpp  # settings.json
    dialogs.cpp         # About + confirm dialogs
    util.cpp            # SHA-256, shell open, args
    resource.rc         # Dialog templates
```
