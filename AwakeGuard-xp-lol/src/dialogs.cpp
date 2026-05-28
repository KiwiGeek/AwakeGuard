#include "dialogs.h"

#include "resource.h"
#include "ui.h"
#include "util.h"
#include "version.h"

#include <cstdio>

namespace {

struct ConfirmContext {
    LPCWSTR title;
    LPCWSTR message;
    LPCWSTR primary;
    LPCWSTR secondary;
    bool showSecondary;
    bool confirmed;
};

enum AboutControlId : int {
    kAboutOk = 1201,
    kAboutLink1 = 1202,
    kAboutLink2 = 1203,
    kAboutLink3 = 1204,
    kAboutLink4 = 1205,
};

constexpr wchar_t kAboutClassName[] = L"AwakeGuardAboutWindow";
constexpr int kAboutWidth = 420;
constexpr int kAboutHeight = 420;

HWND CreateAboutLabel(HWND parent, int x, int y, int w, int h, LPCWSTR text, HFONT font, bool secondary) {
    const HWND label = CreateWindowExW(
        0,
        L"STATIC",
        text,
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | (secondary ? SS_WORDELLIPSIS : 0),
        x,
        y,
        w,
        h,
        parent,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);
    UiApplyFont(label, font);
    if (secondary) {
        SetWindowLongPtrW(label, GWLP_USERDATA, 1);
    }
    return label;
}

HWND CreateAboutButton(HWND parent, int id, int x, int y, int w, int h, LPCWSTR text) {
    const HWND button = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr),
        nullptr);
    UiApplyFont(button, UiBodyFont());
    return button;
}

void CenterWindowOnOwner(HWND window, HWND owner) {
    RECT windowRect {};
    GetWindowRect(window, &windowRect);
    const int windowWidth = windowRect.right - windowRect.left;
    const int windowHeight = windowRect.bottom - windowRect.top;

    RECT ownerRect {};
    if (owner && IsWindowVisible(owner)) {
        GetWindowRect(owner, &ownerRect);
    } else {
        ownerRect.left = 0;
        ownerRect.top = 0;
        ownerRect.right = GetSystemMetrics(SM_CXSCREEN);
        ownerRect.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - windowWidth) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - windowHeight) / 2;
    SetWindowPos(window, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
}

void LayoutAboutWindow(HWND window) {
    const int margin = 24;
    const int contentWidth = kAboutWidth - margin * 2;
    int y = margin;

    CreateAboutLabel(window, margin, y, contentWidth, 32, L"AwakeGuard XP LOL", UiTitleFont(), false);
    y += 38;

    CreateAboutLabel(
        window,
        margin,
        y,
        contentWidth,
        64,
        L"Museum edition for Windows XP. Keeps the PC awake (SetThreadExecutionState) and can disable the screensaver. Built with MinGW for the bit.",
        UiBodyFont(),
        true);
    y += 70;

    wchar_t versionLine[48];
    swprintf(
        versionLine,
        L"Version %u.%u.%u (XP LOL)",
        static_cast<unsigned>(AWAKEGUARD_VERSION_MAJOR),
        static_cast<unsigned>(AWAKEGUARD_VERSION_MINOR),
        static_cast<unsigned>(AWAKEGUARD_VERSION_PATCH));
    CreateAboutLabel(window, margin, y, contentWidth, 18, versionLine, UiBodyFont(), true);
    y += 28;

    CreateAboutLabel(window, margin, y, contentWidth, 18, L"Credits", UiSectionFont(), false);
    y += 24;

    CreateAboutButton(window, kAboutLink1, margin, y, contentWidth, 24, L"GitHub: @kiwigeek");
    y += 28;
    CreateAboutButton(window, kAboutLink2, margin, y, contentWidth, 24, L"Email: joshua@penman.dev");
    y += 28;
    CreateAboutButton(window, kAboutLink3, margin, y, contentWidth, 24, L"Ko-fi: joshpenman");
    y += 28;
    CreateAboutButton(window, kAboutLink4, margin, y, contentWidth, 24, L"joshua.penman.dev");

    const HWND okButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        kAboutWidth - margin - 96,
        kAboutHeight - margin - 36,
        96,
        32,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAboutOk)),
        GetModuleHandleW(nullptr),
        nullptr);
    UiApplyFont(okButton, UiBodyFont());
}

LRESULT CALLBACK AboutWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case kAboutOk:
            DestroyWindow(window);
            return 0;
        case kAboutLink1:
            ShellOpen(L"https://github.com/kiwigeek");
            return 0;
        case kAboutLink2:
            ShellOpen(L"mailto:joshua@penman.dev");
            return 0;
        case kAboutLink3:
            ShellOpen(L"https://ko-fi.com/joshpenman");
            return 0;
        case kAboutLink4:
            ShellOpen(L"https://joshua.penman.dev");
            return 0;
        }
        break;
    case WM_CTLCOLORSTATIC: {
        const HDC dc = reinterpret_cast<HDC>(wParam);
        const HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);
        if (GetWindowLongPtrW(control, GWLP_USERDATA) != 0) {
            SetTextColor(dc, UiSecondaryTextColor());
        } else {
            SetTextColor(dc, UiTextColor());
        }
        return reinterpret_cast<LRESULT>(UiBackgroundBrush());
    }
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

void EnsureAboutClassRegistered() {
    static bool registered = false;
    if (registered) {
        return;
    }

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = AboutWindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = UiBackgroundBrush();
    wc.lpszClassName = kAboutClassName;
    RegisterClassExW(&wc);
    registered = true;
}

INT_PTR CALLBACK ConfirmDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    ConfirmContext* ctx = reinterpret_cast<ConfirmContext*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG: {
        ctx = reinterpret_cast<ConfirmContext*>(lParam);
        SetWindowLongPtrW(dlg, DWLP_USER, reinterpret_cast<LONG_PTR>(ctx));
        SetWindowTextW(dlg, ctx->title);
        SetDlgItemTextW(dlg, IDC_CONFIRM_MESSAGE, ctx->message);
        SetDlgItemTextW(dlg, IDC_CONFIRM_PRIMARY, ctx->primary);
        if (ctx->showSecondary) {
            SetDlgItemTextW(dlg, IDC_CONFIRM_SECONDARY, ctx->secondary);
            ShowWindow(GetDlgItem(dlg, IDC_CONFIRM_SECONDARY), SW_SHOW);
        } else {
            ShowWindow(GetDlgItem(dlg, IDC_CONFIRM_SECONDARY), SW_HIDE);
        }
        UiApplyFontToChildren(dlg, UiBodyFont());
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_CONFIRM_PRIMARY:
            if (ctx) {
                ctx->confirmed = true;
            }
            EndDialog(dlg, IDOK);
            return TRUE;
        case IDC_CONFIRM_SECONDARY:
        case IDCANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(dlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

}  // namespace

bool DialogsShowConfirm(HWND owner, LPCWSTR title, LPCWSTR message, LPCWSTR primary, LPCWSTR secondary, bool topmost) {
    ConfirmContext ctx {title, message, primary, secondary, secondary != nullptr && secondary[0] != L'\0', false};
    const INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_CONFIRM),
        owner,
        ConfirmDlgProc,
        reinterpret_cast<LPARAM>(&ctx));
    (void)result;
    if (topmost && owner) {
        SetForegroundWindow(owner);
    }
    return ctx.confirmed;
}

void DialogsShowError(HWND owner, LPCWSTR title, LPCWSTR message, bool topmost) {
    ConfirmContext ctx {title, message, L"OK", L"", false, false};
    DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_CONFIRM),
        owner,
        ConfirmDlgProc,
        reinterpret_cast<LPARAM>(&ctx));
    if (topmost && owner) {
        SetForegroundWindow(owner);
    }
    (void)title;
}

void DialogsShowAbout(HWND owner) {
    EnsureAboutClassRegistered();

    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_POPUP;
    const HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kAboutClassName,
        L"About AwakeGuard XP LOL",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        kAboutWidth,
        kAboutHeight,
        owner,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    if (!window) {
        return;
    }

    LayoutAboutWindow(window);
    CenterWindowOnOwner(window, owner);
    UiRunModalLoop(window, owner);
}
