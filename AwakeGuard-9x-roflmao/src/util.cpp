#include "util.h"

#include <shellapi.h>

extern "C" int memcmp(const void* a, const void* b, size_t n);

TextStr FormatWin32Error(DWORD code) {
    char* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD len = FormatMessageA(flags, nullptr, code, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    TextStr message;
    if (len && buffer) {
        message.Append(buffer, static_cast<int>(len));
    } else {
        message = "Unknown error";
    }
    if (buffer) {
        LocalFree(buffer);
    }
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
        message.PopBack();
    }
    return message;
}

PathStr GetExePath() {
    PathStr path;
    const DWORD len = GetModuleFileNameA(nullptr, path.Buffer(), static_cast<DWORD>(PathStr::Capacity()));
    if (len == 0 || len >= static_cast<DWORD>(PathStr::Capacity())) {
        return PathStr();  // Win9x paths are limited to MAX_PATH; treat truncation as failure
    }
    path.SyncLength();
    return path;
}

namespace {

// Static (not stack) buffers: frames over 4 KB would need __chkstk from libgcc.
char g_compareBufferA[16 * 1024];
char g_compareBufferB[16 * 1024];

bool CompareFileBytes(const char* a, const char* b) {
    HANDLE ha = CreateFileA(a, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE hb = CreateFileA(b, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (ha == INVALID_HANDLE_VALUE || hb == INVALID_HANDLE_VALUE) {
        if (ha != INVALID_HANDLE_VALUE) {
            CloseHandle(ha);
        }
        if (hb != INVALID_HANDLE_VALUE) {
            CloseHandle(hb);
        }
        return false;
    }

    bool match = true;
    for (;;) {
        DWORD readA = 0;
        DWORD readB = 0;
        const BOOL okA = ReadFile(ha, g_compareBufferA, sizeof(g_compareBufferA), &readA, nullptr);
        const BOOL okB = ReadFile(hb, g_compareBufferB, sizeof(g_compareBufferB), &readB, nullptr);
        if (!okA || !okB || readA != readB) {
            match = false;
            break;
        }
        if (readA == 0) {
            break;
        }
        if (memcmp(g_compareBufferA, g_compareBufferB, readA) != 0) {
            match = false;
            break;
        }
    }

    CloseHandle(ha);
    CloseHandle(hb);
    return match;
}

}  // namespace

bool FilesIdentical(const char* a, const char* b) {
    WIN32_FIND_DATAA fa {};
    WIN32_FIND_DATAA fb {};
    HANDLE ha = FindFirstFileA(a, &fa);
    HANDLE hb = FindFirstFileA(b, &fb);
    if (ha == INVALID_HANDLE_VALUE || hb == INVALID_HANDLE_VALUE) {
        if (ha != INVALID_HANDLE_VALUE) {
            FindClose(ha);
        }
        if (hb != INVALID_HANDLE_VALUE) {
            FindClose(hb);
        }
        return false;
    }
    FindClose(ha);
    FindClose(hb);

    if (fa.nFileSizeLow != fb.nFileSizeLow || fa.nFileSizeHigh != fb.nFileSizeHigh) {
        return false;
    }

    if (fa.nFileSizeLow == 0 && fa.nFileSizeHigh == 0) {
        return true;
    }

    return CompareFileBytes(a, b);
}

bool DeletePathAllowReadonly(const char* path, TextStr& errorOut) {
    errorOut.Clear();
    if (!DeleteFileA(path)) {
        const DWORD code = GetLastError();
        if (code == ERROR_FILE_NOT_FOUND) {
            return true;
        }

        if (code == ERROR_ACCESS_DENIED) {
            const DWORD attrs = GetFileAttributesA(path);
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
                SetFileAttributesA(path, attrs & ~FILE_ATTRIBUTE_READONLY);
                if (DeleteFileA(path)) {
                    return true;
                }
            }
        }

        errorOut = FormatWin32Error(GetLastError());
        return false;
    }

    return true;
}

void ShellOpen(const char* target) {
    ShellExecuteA(nullptr, "open", target, nullptr, nullptr, SW_SHOWNORMAL);
}

TextStr QuoteCommandArg(const char* value) {
    PathStr trimmed = value;
    while (!trimmed.empty() && (trimmed.back() == '\\' || trimmed.back() == '/')) {
        trimmed.PopBack();
    }
    TextStr out = "\"";
    for (const char* p = trimmed.c_str(); *p; ++p) {
        if (*p == '"') {
            out += "\\\"";
        } else {
            out += *p;
        }
    }
    out += "\"";
    return out;
}
