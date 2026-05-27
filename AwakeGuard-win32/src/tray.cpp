#include "tray.h"

#include "app.h"
#include "dialogs.h"
#include "session.h"
#include "settings_wnd.h"
#include "session.h"
#include "util.h"

namespace {

HICON CreateCircleIcon(COLORREF color) {
    constexpr int size = 32;
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);

    BITMAPV5HEADER bi {};
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = size;
    bi.bV5Height = size;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    void* bits = nullptr;
    HBITMAP colorBitmap = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP maskBitmap = CreateBitmap(size, size, 1, 1, nullptr);
    SelectObject(dc, colorBitmap);

    const UINT32 fill = 0x00000000;
  for (int i = 0; i < size * size; ++i) {
        static_cast<UINT32*>(bits)[i] = fill;
    }

    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    Ellipse(dc, 2, 2, size - 2, size - 2);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    ICONINFO info {};
    info.fIcon = TRUE;
    info.hbmColor = colorBitmap;
    info.hbmMask = maskBitmap;
    HICON icon = CreateIconIndirect(&info);

    DeleteObject(colorBitmap);
    DeleteObject(maskBitmap);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    return icon;
}

void UpdateTrayMenuState(AppState& app) {
    if (!app.trayMenu) {
        return;
    }
    const std::wstring status = SessionStatusText(app.session);
    ModifyMenuW(app.trayMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, status.c_str());
    EnableMenuItem(app.trayMenu, 2, MF_BYCOMMAND | (app.session.isActive ? MF_ENABLED : MF_GRAYED));
}

HMENU BuildTrayMenu(AppState& app) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, L"Loading...");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 1, L"Open settings\u2026");
    AppendMenuW(menu, MF_STRING, 2, L"Turn off");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    HMENU durations = CreatePopupMenu();
    for (int i = 0; i < kDurationPresetCount; ++i) {
        AppendMenuW(durations, MF_STRING, 100 + i, kDurationPresets[i].label);
    }
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(durations), L"Turn on for");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 3, L"About AwakeGuard\u2026");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 4, L"Exit");
    UpdateTrayMenuState(app);
    return menu;
}

void TurnOnForPreset(AppState& app, int presetIndex) {
    app.session.selectedDurationIndex = presetIndex;
    SendMessageW(app.cmbDuration, CB_SETCURSEL, presetIndex, 0);
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
    app.iconActive = CreateCircleIcon(RGB(16, 185, 129));
    app.iconInactive = CreateCircleIcon(RGB(120, 120, 120));
    app.trayMenu = BuildTrayMenu(app);

    app.trayData.cbSize = sizeof(NOTIFYICONDATAW);
    app.trayData.hWnd = app.settingsWindow;
    app.trayData.uID = 1;
    app.trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    app.trayData.uCallbackMessage = WM_TRAYICON;
    app.trayData.hIcon = app.iconInactive;
    wcscpy_s(app.trayData.szTip, L"AwakeGuard");
    Shell_NotifyIconW(NIM_ADD, &app.trayData);
    app.trayData.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &app.trayData);
}

void TrayDispose(AppState& app) {
    app.trayData.uFlags = 0;
    Shell_NotifyIconW(NIM_DELETE, &app.trayData);
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
    const std::wstring status = SessionStatusText(app.session);
    app.trayData.hIcon = app.session.isActive ? app.iconActive : app.iconInactive;
    app.trayData.uFlags = NIF_ICON | NIF_TIP;
    wcscpy_s(app.trayData.szTip, (L"AwakeGuard \x2014 " + status).c_str());
    Shell_NotifyIconW(NIM_MODIFY, &app.trayData);
    UpdateTrayMenuState(app);
}

void TrayShowBalloon(AppState& app, LPCWSTR message) {
    app.trayData.uFlags = NIF_INFO;
    wcscpy_s(app.trayData.szInfoTitle, L"AwakeGuard");
    wcscpy_s(app.trayData.szInfo, message);
    app.trayData.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &app.trayData);
    app.trayData.uFlags = NIF_ICON | NIF_TIP;
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
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
