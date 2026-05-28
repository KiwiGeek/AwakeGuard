#include "ui.h"

namespace {

HFONT g_bodyFont = nullptr;
HFONT g_titleFont = nullptr;
HFONT g_sectionFont = nullptr;
HBRUSH g_backgroundBrush = nullptr;
HBRUSH g_panelBrush = nullptr;

typedef BOOL(WINAPI* InitCommonControlsExFn)(LPINITCOMMONCONTROLSEX);

HFONT CreateFontPointSize(int pointSize, int weight, LPCSTR faceName) {
    const HDC screen = GetDC(nullptr);
    const int logPixelsY = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(nullptr, screen);

    return CreateFontA(
        -MulDiv(pointSize, logPixelsY, 72),
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        faceName);
}

void RefreshBrushes() {
    if (g_backgroundBrush) {
        DeleteObject(g_backgroundBrush);
    }
    if (g_panelBrush) {
        DeleteObject(g_panelBrush);
    }

    g_backgroundBrush = CreateSolidBrush(RGB(249, 249, 249));
    g_panelBrush = CreateSolidBrush(RGB(243, 243, 243));
}

}  // namespace

void UiInitialize() {
    HMODULE commctrl = LoadLibraryA("COMCTL32.DLL");
    if (commctrl) {
        InitCommonControlsExFn initEx =
            reinterpret_cast<InitCommonControlsExFn>(GetProcAddress(commctrl, "InitCommonControlsEx"));
        if (initEx) {
            INITCOMMONCONTROLSEX icc {};
            icc.dwSize = sizeof(icc);
            icc.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
            initEx(&icc);
        } else {
            InitCommonControls();
        }
        FreeLibrary(commctrl);
    } else {
        InitCommonControls();
    }

    g_bodyFont = CreateFontPointSize(10, FW_NORMAL, "MS Sans Serif");
    g_titleFont = CreateFontPointSize(16, FW_BOLD, "MS Sans Serif");
    g_sectionFont = CreateFontPointSize(10, FW_BOLD, "MS Sans Serif");
    RefreshBrushes();
}

void UiShutdown() {
    if (g_bodyFont) {
        DeleteObject(g_bodyFont);
        g_bodyFont = nullptr;
    }
    if (g_titleFont) {
        DeleteObject(g_titleFont);
        g_titleFont = nullptr;
    }
    if (g_sectionFont) {
        DeleteObject(g_sectionFont);
        g_sectionFont = nullptr;
    }
    if (g_backgroundBrush) {
        DeleteObject(g_backgroundBrush);
        g_backgroundBrush = nullptr;
    }
    if (g_panelBrush) {
        DeleteObject(g_panelBrush);
        g_panelBrush = nullptr;
    }
}

HFONT UiBodyFont() {
    return g_bodyFont;
}

HFONT UiTitleFont() {
    return g_titleFont;
}

HFONT UiSectionFont() {
    return g_sectionFont;
}

void UiApplyFont(HWND hwnd, HFONT font) {
    SendMessageA(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void UiApplyFontToChildren(HWND parent, HFONT font) {
    for (HWND child = GetWindow(parent, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        UiApplyFont(child, font);
    }
}

void UiApplyModernChrome(HWND) {
}

COLORREF UiTextColor() {
    return RGB(32, 32, 32);
}

COLORREF UiSecondaryTextColor() {
    return RGB(96, 96, 96);
}

HBRUSH UiBackgroundBrush() {
    return g_backgroundBrush;
}

HBRUSH UiPanelBrush() {
    return g_panelBrush;
}

INT_PTR UiRunModalLoop(HWND dialog, HWND owner) {
    if (owner) {
        EnableWindow(owner, FALSE);
    }

    ShowWindow(dialog, SW_SHOW);
    SetForegroundWindow(dialog);

    MSG msg {};
    while (IsWindow(dialog) && GetMessageA(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_QUIT) {
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }

        if (!IsDialogMessageA(dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (msg.message == WM_NULL && !IsWindow(dialog)) {
            break;
        }
    }

    if (owner) {
        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);
    }

    return 0;
}
