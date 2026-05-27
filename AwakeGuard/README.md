# AwakeGuard

A Windows system-tray app built with **WPF** and **[WPF-UI](https://github.com/lepoco/wpfui)** (Fluent design). It temporarily prevents your PC from sleeping and from starting the screensaver or locking the display.

## Features

- **System tray** — runs in the background; close the window to hide (does not exit).
- **Two protections** (can be enabled separately):
  - Prevent PC from sleeping
  - Prevent screensaver / display lock (display stays “required”)
- **Durations**: 30 minutes, 1 hour, 2 hours, 4 hours, 8 hours, rest of today (until midnight), or until you turn it off.
- **Tray menu** — quick “Turn on for …” presets, status line, turn off, exit.
- **Left-click tray icon** — toggle on/off with the last settings.
- **Double-click tray icon** — open the settings window.

## How it works

Windows [`SetThreadExecutionState`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate) is used with `ES_SYSTEM_REQUIRED` (sleep) and `ES_DISPLAY_REQUIRED` (display/screensaver). This is the same mechanism used by “keep awake” utilities; it does not change power plan settings permanently.

## Requirements

- Windows 10 or 11
- [.NET 10 SDK](https://dotnet.microsoft.com/download) (or change `TargetFramework` in the `.csproj` to `net8.0-windows` if you prefer)

## Build and run

```powershell
cd C:\Users\joshua\AwakeGuard\AwakeGuard
dotnet run
```

Release build:

```powershell
dotnet publish -c Release -r win-x64 --self-contained false
```

The executable is under `bin\Release\net10.0-windows\publish\`.

## Optional: run at startup

Create a shortcut to `AwakeGuard.exe` in:

`%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup`

## Notes

- Stopping the app or choosing **Turn off** restores normal Windows power/lock behavior immediately.
- Timed sessions show a tray balloon when they expire.
- This does not block manual sleep (Start menu → Sleep) or manual lock (Win+L).
