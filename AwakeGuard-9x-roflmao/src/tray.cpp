#include "tray.h"

#include "app.h"
#include "dialogs.h"
#include "os_compat.h"
#include "resource.h"
#include "session.h"
#include "settings_wnd.h"
#include "util.h"

#include <stddef.h>

namespace {

HICON LoadTrayIcon(int resourceId) {
    HICON icon = static_cast<HICON>(LoadImageA(
        GetModuleHandleA(nullptr),
        MAKEINTRESOURCE(resourceId),
        IMAGE_ICON,
        16,
        16,
        LR_DEFAULTCOLOR));
    if (!icon) {
        icon = LoadIconA(GetModuleHandleA(nullptr), MAKEINTRESOURCE(resourceId));
    }
    return icon;
}

void UpdateTrayMenuState(AppState& app) {
    if (!app.trayMenu) {
        return;
    }
    const std::string status = SessionStatusText(app.session);
    ModifyMenuA(app.trayMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, status.c_str());
    EnableMenuItem(app.trayMenu, 2, MF_BYCOMMAND | (app.session.isActive ? MF_ENABLED : MF_GRAYED));
}

HMENU BuildTrayMenu(AppState& app) {
    HMENU menu = CreatePopupMenu();
    AppendMenuA(menu, MF_STRING | MF_GRAYED, 0, "Loading...");
    AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(menu, MF_STRING, 1, "Open settings...");
    AppendMenuA(menu, MF_STRING, 2, "Turn off");
    AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);

    HMENU durations = CreatePopupMenu();
    for (int i = 0; i < kDurationPresetCount; ++i) {
        AppendMenuA(durations, MF_STRING, 100 + i, kDurationPresets[i].label);
    }
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(durations), "Turn on for");
    AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(menu, MF_STRING, 3, "About AwakeGuard 9x ROFLMAO...");
    AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(menu, MF_STRING, 4, "Exit");
    UpdateTrayMenuState(app);
    return menu;
}

void TurnOnForPreset(AppState& app, int presetIndex) {
    app.session.selectedDurationIndex = presetIndex;
    SendMessageA(app.cmbDuration, CB_SETCURSEL, presetIndex, 0);
    if (app.session.isActive) {
        SessionStop(app.session);
        AppOnSessionChanged(app, false);
    }
    SessionStart(app.session, presetIndex);
    AppOnSessionChanged(app, false);
    SetTimer(app.settingsWindow, IDT_REFRESH, REFRESH_MS, nullptr);
    SetTimer(app.settingsWindow, IDT_EXPIRY, EXPIRY_MS, nullptr);
}

}  // namespace

void TrayInitialize(AppState& app) {
    app.iconActive = LoadTrayIcon(IDI_TRAY_ACTIVE);
    app.iconInactive = LoadTrayIcon(IDI_TRAY_INACTIVE);
    app.trayMenu = BuildTrayMenu(app);

    ZeroMemory(&app.trayData, sizeof(app.trayData));
    app.trayData.cbSize = OsNotifyIconDataSize(false);
    app.trayData.hWnd = app.settingsWindow;
    app.trayData.uID = 1;
    app.trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    app.trayData.uCallbackMessage = WM_TRAYICON;
    app.trayData.hIcon = app.iconInactive;
    lstrcpynA(app.trayData.szTip, "AwakeGuard 9x ROFLMAO", static_cast<int>(sizeof(app.trayData.szTip) / sizeof(char)));
    Shell_NotifyIconA(NIM_ADD, &app.trayData);
}

void TrayDispose(AppState& app) {
    app.trayData.uFlags = 0;
    Shell_NotifyIconA(NIM_DELETE, &app.trayData);
    if (app.trayMenu) {
        if (HMENU durations = GetSubMenu(app.trayMenu, 5)) {
            DestroyMenu(durations);
        }
        DestroyMenu(app.trayMenu);
        app.trayMenu = nullptr;
    }
    if (app.iconActive) {
        DestroyIcon(app.iconActive);
        app.iconActive = nullptr;
    }
    if (app.iconInactive) {
        DestroyIcon(app.iconInactive);
        app.iconInactive = nullptr;
    }
}

void TrayUpdate(AppState& app) {
    const std::string status = SessionStatusText(app.session);
    app.trayData.cbSize = OsNotifyIconDataSize(false);
    app.trayData.hIcon = app.session.isActive ? app.iconActive : app.iconInactive;
    app.trayData.uFlags = NIF_ICON | NIF_TIP;
    const std::string tip = "AwakeGuard 9x ROFLMAO - " + status;
    lstrcpynA(app.trayData.szTip, tip.c_str(), static_cast<int>(sizeof(app.trayData.szTip) / sizeof(char)));
    Shell_NotifyIconA(NIM_MODIFY, &app.trayData);
    UpdateTrayMenuState(app);
}

void TrayShowBalloon(AppState& app, LPCSTR message) {
    if (!OsSupportsTrayBalloons()) {
        return;
    }
    app.trayData.cbSize = OsNotifyIconDataSize(true);
    app.trayData.uFlags = NIF_INFO;
    lstrcpynA(app.trayData.szInfoTitle, "AwakeGuard 9x ROFLMAO", static_cast<int>(sizeof(app.trayData.szInfoTitle) / sizeof(char)));
    lstrcpynA(app.trayData.szInfo, message, static_cast<int>(sizeof(app.trayData.szInfo) / sizeof(char)));
    app.trayData.dwInfoFlags = NIIF_INFO;
    app.trayData.uTimeout = 10000;
    Shell_NotifyIconA(NIM_MODIFY, &app.trayData);
    app.trayData.uFlags = NIF_ICON | NIF_TIP;
    app.trayData.cbSize = OsNotifyIconDataSize(false);
}

LRESULT TrayHandleMessage(AppState& app, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
            AppToggleSession(app);
            return 0;
        case WM_LBUTTONDBLCLK:
            SettingsWindowShow(app, true);
            return 0;
        case WM_CONTEXTMENU:
        case WM_RBUTTONUP: {
            POINT pt {};
            GetCursorPos(&pt);
            UpdateTrayMenuState(app);
            SetForegroundWindow(hwnd);
            const UINT cmd = TrackPopupMenu(app.trayMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
            PostMessageW(hwnd, WM_NULL, 0, 0);
            switch (cmd) {
            case 1:
                SettingsWindowShow(app, true);
                break;
            case 2:
                if (app.session.isActive) {
                    SessionStop(app.session);
                    KillTimer(hwnd, IDT_REFRESH);
                    KillTimer(hwnd, IDT_EXPIRY);
                    AppOnSessionChanged(app, true);
                }
                break;
            case 3:
                DialogsShowAbout(app.settingsWindow);
                break;
            case 4:
                PostQuitMessage(0);
                break;
            default:
                if (cmd >= 100 && cmd < 100 + kDurationPresetCount) {
                    TurnOnForPreset(app, static_cast<int>(cmd - 100));
                }
                break;
            }
            return 0;
        }
        }
        break;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
