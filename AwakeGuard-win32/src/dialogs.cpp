#include "dialogs.h"

#include "resource.h"
#include "ui.h"
#include "util.h"

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
constexpr int kAboutHeight = 400;

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

HWND CreateAboutLink(HWND parent, int id, int x, int y, int w, int h, LPCWSTR markup) {
    const HWND link = CreateWindowExW(
        0,
        WC_LINK,
        markup,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr),
        nullptr);
    UiApplyFont(link, UiBodyFont());
    return link;
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

    CreateAboutLabel(window, margin, y, contentWidth, 32, L"AwakeGuard", UiTitleFont(), false);
    y += 38;

    CreateAboutLabel(
        window,
        margin,
        y,
        contentWidth,
        52,
        L"A system-tray utility that temporarily keeps your PC awake and prevents automatic sleep, screensaver, and display lock.",
        UiBodyFont(),
        true);
    y += 58;

    CreateAboutLabel(window, margin, y, contentWidth, 18, L"Version 1.0.0", UiBodyFont(), true);
    y += 28;

    CreateAboutLabel(window, margin, y, contentWidth, 18, L"Credits", UiSectionFont(), false);
    y += 24;

    CreateAboutLink(
        window,
        kAboutLink1,
        margin,
        y,
        contentWidth,
        18,
        L"<a href=\"https://github.com/kiwigeek\">@kiwigeek</a> on GitHub");
    y += 22;
    CreateAboutLink(
        window,
        kAboutLink2,
        margin,
        y,
        contentWidth,
        18,
        L"<a href=\"mailto:joshua@penman.dev\">joshua@penman.dev</a>");
    y += 22;
    CreateAboutLink(
        window,
        kAboutLink3,
        margin,
        y,
        contentWidth,
        18,
        L"<a href=\"https://ko-fi.com/joshpenman\">Support on Ko-fi</a>");
    y += 22;
    CreateAboutLink(
        window,
        kAboutLink4,
        margin,
        y,
        contentWidth,
        18,
        L"<a href=\"https://joshua.penman.dev\">joshua.penman.dev</a>");

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

bool HandleAboutLinkClick(const NMHDR* header) {
    if (!header || (header->code != NM_CLICK && header->code != NM_RETURN)) {
        return false;
    }

    switch (header->idFrom) {
    case kAboutLink1:
        ShellOpen(L"https://github.com/kiwigeek");
        return true;
    case kAboutLink2:
        ShellOpen(L"mailto:joshua@penman.dev");
        return true;
    case kAboutLink3:
        ShellOpen(L"https://ko-fi.com/joshpenman");
        return true;
    case kAboutLink4:
        ShellOpen(L"https://joshua.penman.dev");
        return true;
    default:
        return false;
    }
}

LRESULT CALLBACK AboutWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == kAboutOk) {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_NOTIFY:
        if (HandleAboutLinkClick(reinterpret_cast<NMHDR*>(lParam))) {
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
        UiApplyModernChrome(dlg);
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
    UNREFERENCED_PARAMETER(result);
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
    UNREFERENCED_PARAMETER(title);
}

void DialogsShowAbout(HWND owner) {
    EnsureAboutClassRegistered();

    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_POPUP;
    const HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kAboutClassName,
        L"About AwakeGuard",
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

    UiApplyModernChrome(window);
    LayoutAboutWindow(window);
    CenterWindowOnOwner(window, owner);
    UiRunModalLoop(window, owner);
}
