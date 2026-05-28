#include "settings_wnd.h"

#include "app.h"
#include "cleanup.h"
#include "os_compat.h"
#include "dialogs.h"
#include "startup.h"
#include "tray.h"
#include "ui.h"
#include "util.h"

namespace {

constexpr char kSettingsClassName[] = "AwakeGuardSettingsWindow";
constexpr int kWindowWidth = 460;
constexpr int kWindowHeight = 560;

enum StaticStyleTag : LONG_PTR {
    kStaticNormal = 0,
    kStaticSecondary = 1,
    kStaticPanel = 2,
};

HWND CreateLabel(HWND parent, int x, int y, int w, int h, LPCSTR text, HFONT font, StaticStyleTag tag) {
    const HWND label = CreateWindowExA(
        0,
        "STATIC",
        text,
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        x,
        y,
        w,
        h,
        parent,
        nullptr,
        GetModuleHandleA(nullptr),
        nullptr);
    UiApplyFont(label, font);
    SetWindowLongPtrA(label, GWLP_USERDATA, tag);
    return label;
}

HWND CreateCheck(HWND parent, int id, int x, int y, int w, int h, LPCSTR text) {
    const HWND check = CreateWindowExA(
        0,
        "BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleA(nullptr),
        nullptr);
    UiApplyFont(check, UiBodyFont());
    return check;
}

HWND CreateButton(HWND parent, int id, int x, int y, int w, int h, LPCSTR text) {
    const HWND button = CreateWindowExA(
        0,
        "BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleA(nullptr),
        nullptr);
    UiApplyFont(button, UiBodyFont());
    return button;
}

void LayoutControls(AppState& app) {
    const int margin = 24;
    const int contentWidth = kWindowWidth - margin * 2;
    int y = margin;

    CreateLabel(
        app.settingsWindow,
        margin,
        y,
        contentWidth,
        36,
        "Keep your PC awake and prevent automatic lock or sleep.",
        UiBodyFont(),
        kStaticSecondary);
    y += 44;

    CreateLabel(app.settingsWindow, margin, y, 200, 20, "Protection", UiSectionFont(), kStaticNormal);
    y += 24;
    app.chkSleep = CreateCheck(app.settingsWindow, IDC_CHK_SLEEP, margin, y, contentWidth, 24, "Prevent PC from sleeping");
    y += 30;
    app.chkDisplay = CreateCheck(app.settingsWindow, IDC_CHK_DISPLAY, margin, y, contentWidth, 24, "Prevent screensaver");
    y += 42;

    CreateLabel(app.settingsWindow, margin, y, 200, 20, "Startup", UiSectionFont(), kStaticNormal);
    y += 24;
    app.chkStartup = CreateCheck(app.settingsWindow, IDC_CHK_STARTUP, margin, y, contentWidth, 24, "Start with Windows");
    y += 42;

    CreateLabel(app.settingsWindow, margin, y, 200, 20, "Duration", UiSectionFont(), kStaticNormal);
    y += 24;
    app.cmbDuration = CreateWindowExA(
        0,
        "ComboBox",
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        margin,
        y,
        contentWidth,
        240,
        app.settingsWindow,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CMB_DURATION)),
        GetModuleHandleA(nullptr),
        nullptr);
    UiApplyFont(app.cmbDuration, UiBodyFont());
    for (int i = 0; i < kDurationPresetCount; ++i) {
        SendMessageA(app.cmbDuration, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kDurationPresets[i].label));
    }
    SendMessageA(app.cmbDuration, CB_SETCURSEL, app.session.selectedDurationIndex, 0);
    y += 40;

    app.txtStatus = CreateWindowExA(
        0,
        "STATIC",
        "",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        margin,
        y,
        contentWidth,
        56,
        app.settingsWindow,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TXT_STATUS)),
        GetModuleHandleA(nullptr),
        nullptr);
    UiApplyFont(app.txtStatus, UiBodyFont());
    SetWindowLongPtrA(app.txtStatus, GWLP_USERDATA, kStaticPanel);
    y += 72;

    CreateButton(app.settingsWindow, IDC_BTN_ABOUT, margin, y, 100, 34, "About");
    CreateButton(app.settingsWindow, IDC_BTN_TURNOFF, kWindowWidth - margin - 200, y, 90, 34, "Turn off");
    CreateButton(app.settingsWindow, IDC_BTN_TURNON, kWindowWidth - margin - 100, y, 90, 34, "Turn on");

    CheckDlgButton(app.settingsWindow, IDC_CHK_SLEEP, app.session.preventSleep ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(app.settingsWindow, IDC_CHK_DISPLAY, app.session.preventDisplayOff ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(app.settingsWindow, IDC_CHK_STARTUP, app.startWithWindows ? BST_CHECKED : BST_UNCHECKED);
}

bool CanTurnOn(const AppState& app) {
    return !app.session.isActive && (app.session.preventSleep || app.session.preventDisplayOff);
}

void UpdateCommandStates(AppState& app) {
    const bool hasSleepApi = OsHasSetThreadExecutionState();
    EnableWindow(app.chkSleep, hasSleepApi ? TRUE : FALSE);
    if (!hasSleepApi && app.session.preventSleep) {
        app.session.preventSleep = false;
        CheckDlgButton(app.settingsWindow, IDC_CHK_SLEEP, BST_UNCHECKED);
    }
    EnableWindow(app.cmbDuration, app.session.isActive ? FALSE : TRUE);
    EnableWindow(GetDlgItem(app.settingsWindow, IDC_BTN_TURNON), CanTurnOn(app) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(app.settingsWindow, IDC_BTN_TURNOFF), app.session.isActive ? TRUE : FALSE);
    SetDlgItemTextA(app.settingsWindow, IDC_TXT_STATUS, SessionStatusText(app.session).c_str());
}

void OnStartupCheckboxChanged(AppState& app) {
    if (app.suppressStartupChange) {
        return;
    }
    const bool enabled = IsDlgButtonChecked(app.settingsWindow, IDC_CHK_STARTUP) == BST_CHECKED;
    if (enabled) {
        const StartupEnableResult result = StartupTryEnable(app, true);
        if (result == StartupEnableResult::Failed) {
            app.suppressStartupChange = true;
            app.startWithWindows = false;
            CheckDlgButton(app.settingsWindow, IDC_CHK_STARTUP, BST_UNCHECKED);
            app.suppressStartupChange = false;
        } else if (result == StartupEnableResult::Relocating) {
            return;
        } else {
            app.startWithWindows = true;
        }
    } else {
        StartupDisable();
        app.startWithWindows = false;
    }
}

LRESULT HandleStaticColor(WPARAM wParam, LPARAM lParam) {
    const HDC dc = reinterpret_cast<HDC>(wParam);
    const HWND control = reinterpret_cast<HWND>(lParam);
    const LONG_PTR tag = GetWindowLongPtrA(control, GWLP_USERDATA);

    SetBkMode(dc, TRANSPARENT);
    if (tag == kStaticPanel) {
        SetTextColor(dc, UiTextColor());
        return reinterpret_cast<LRESULT>(UiPanelBrush());
    }

    SetTextColor(dc, tag == kStaticSecondary ? UiSecondaryTextColor() : UiTextColor());
    return reinterpret_cast<LRESULT>(UiBackgroundBrush());
}

}  // namespace

bool SettingsWindowCreate(AppState& app) {
    WNDCLASSEXA wc {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsWindowProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = UiBackgroundBrush();
    wc.lpszClassName = kSettingsClassName;
    RegisterClassExA(&wc);

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    app.settingsWindow = CreateWindowExA(
        0,
        kSettingsClassName,
        "AwakeGuard 9x ROFLMAO",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        kWindowWidth,
        kWindowHeight,
        nullptr,
        nullptr,
        wc.hInstance,
        &app);

    if (!app.settingsWindow) {
        return false;
    }

    UiApplyModernChrome(app.settingsWindow);
    LayoutControls(app);
    UpdateCommandStates(app);
    return true;
}

void SettingsWindowShow(AppState& app, bool refreshStartupState) {
    if (refreshStartupState) {
        AppApplyStartupState(app);
    }
    ShowWindow(app.settingsWindow, SW_SHOW);
    SetForegroundWindow(app.settingsWindow);
    UpdateCommandStates(app);
}

void SettingsWindowHide(AppState& app) {
    ShowWindow(app.settingsWindow, SW_HIDE);
}

void SettingsWindowRefresh(AppState& app) {
    UpdateCommandStates(app);
    TrayUpdate(app);
}

LRESULT CALLBACK SettingsWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* app = reinterpret_cast<AppState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (msg == WM_CREATE) {
        const auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
        app = reinterpret_cast<AppState*>(cs->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        return 0;
    }

    if (!app) {
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            return 0;
        }
        break;
    case WM_CTLCOLORSTATIC:
        return HandleStaticColor(wParam, lParam);
    case WM_TIMER:
        if (wParam == IDT_REFRESH) {
            SessionRefresh(app->session);
            SettingsWindowRefresh(*app);
            return 0;
        }
        if (wParam == IDT_EXPIRY) {
            bool expired = false;
            SessionCheckExpiry(app->session, expired);
            SettingsWindowRefresh(*app);
            if (expired) {
                KillTimer(hwnd, IDT_REFRESH);
                KillTimer(hwnd, IDT_EXPIRY);
                AppOnSessionChanged(*app, true);
            }
            return 0;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_CHK_SLEEP:
            if (HIWORD(wParam) == BN_CLICKED) {
                app->session.preventSleep = IsDlgButtonChecked(hwnd, IDC_CHK_SLEEP) == BST_CHECKED;
                if (app->session.isActive) {
                    SessionRefresh(app->session);
                }
                SettingsWindowRefresh(*app);
            }
            return 0;
        case IDC_CHK_DISPLAY:
            if (HIWORD(wParam) == BN_CLICKED) {
                app->session.preventDisplayOff = IsDlgButtonChecked(hwnd, IDC_CHK_DISPLAY) == BST_CHECKED;
                if (app->session.isActive) {
                    SessionRefresh(app->session);
                }
                SettingsWindowRefresh(*app);
            }
            return 0;
        case IDC_CHK_STARTUP:
            if (HIWORD(wParam) == BN_CLICKED) {
                OnStartupCheckboxChanged(*app);
            }
            return 0;
        case IDC_BTN_ABOUT:
            DialogsShowAbout(hwnd);
            return 0;
        case IDC_BTN_TURNOFF:
            if (app->session.isActive) {
                SessionStop(app->session);
                KillTimer(hwnd, IDT_REFRESH);
                KillTimer(hwnd, IDT_EXPIRY);
                AppOnSessionChanged(*app, true);
            }
            return 0;
        case IDC_BTN_TURNON:
            if (CanTurnOn(*app)) {
                const int index = static_cast<int>(SendMessageA(app->cmbDuration, CB_GETCURSEL, 0, 0));
                SessionStart(app->session, index);
                SetTimer(hwnd, IDT_REFRESH, REFRESH_MS, nullptr);
                SetTimer(hwnd, IDT_EXPIRY, EXPIRY_MS, nullptr);
                AppOnSessionChanged(*app, false);
            }
            return 0;
        }
        break;
    case WM_CLOSE:
        SettingsWindowHide(*app);
        return 0;
    case WM_TRAYICON:
        return TrayHandleMessage(*app, hwnd, msg, wParam, lParam);
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
