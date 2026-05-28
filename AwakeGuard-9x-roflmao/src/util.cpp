#include "util.h"

#include <shellapi.h>
#include <vector>

extern int __argc;
extern char** __argv;

std::string FormatWin32Error(DWORD code) {
    char* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD len = FormatMessageA(flags, nullptr, code, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    std::string message = len ? std::string(buffer, len) : "Unknown error";
    if (buffer) {
        LocalFree(buffer);
    }
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
        message.pop_back();
    }
    return message;
}

std::string GetExePath() {
    std::string path(MAX_PATH, '\0');
    for (;;) {
        const DWORD len = GetModuleFileNameA(nullptr, &path[0], static_cast<DWORD>(path.size()));
        if (len == 0) {
            return "";
        }
        if (len < path.size() - 1) {
            path.resize(len);
            return path;
        }
        path.resize(path.size() * 2);
    }
}

std::string GetExeDirectory() {
    const std::string exe = GetExePath();
    const size_t pos = exe.find_last_of("\\/");
    return pos == std::string::npos ? "" : exe.substr(0, pos);
}

namespace {

bool CompareFileBytes(const std::string& a, const std::string& b) {
    HANDLE ha = CreateFileA(a.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE hb = CreateFileA(b.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (ha == INVALID_HANDLE_VALUE || hb == INVALID_HANDLE_VALUE) {
        if (ha != INVALID_HANDLE_VALUE) {
            CloseHandle(ha);
        }
        if (hb != INVALID_HANDLE_VALUE) {
            CloseHandle(hb);
        }
        return false;
    }

    std::vector<char> bufferA(64 * 1024);
    std::vector<char> bufferB(64 * 1024);
    bool match = true;

    for (;;) {
        DWORD readA = 0;
        DWORD readB = 0;
        const BOOL okA = ReadFile(ha, bufferA.data(), static_cast<DWORD>(bufferA.size()), &readA, nullptr);
        const BOOL okB = ReadFile(hb, bufferB.data(), static_cast<DWORD>(bufferB.size()), &readB, nullptr);
        if (!okA || !okB || readA != readB) {
            match = false;
            break;
        }
        if (readA == 0) {
            break;
        }
        if (memcmp(bufferA.data(), bufferB.data(), readA) != 0) {
            match = false;
            break;
        }
    }

    CloseHandle(ha);
    CloseHandle(hb);
    return match;
}

}  // namespace

bool FileSha256Hex(const std::string& path, std::string& hexOut) {
    (void)path;
    hexOut.clear();
    return false;
}

bool FilesIdentical(const std::string& a, const std::string& b) {
    WIN32_FIND_DATAA fa {};
    WIN32_FIND_DATAA fb {};
    HANDLE ha = FindFirstFileA(a.c_str(), &fa);
    HANDLE hb = FindFirstFileA(b.c_str(), &fb);
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

bool GetFileSizeBytes(const std::string& path, ULONGLONG& sizeOut) {
    WIN32_FIND_DATAA info {};
    HANDLE find = FindFirstFileA(path.c_str(), &info);
    if (find == INVALID_HANDLE_VALUE) {
        return false;
    }
    FindClose(find);
    sizeOut = (static_cast<ULONGLONG>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    return true;
}

bool DeletePathAllowReadonly(const std::string& path, std::string& errorOut) {
    errorOut.clear();
    if (!DeleteFileA(path.c_str())) {
        const DWORD code = GetLastError();
        if (code == ERROR_FILE_NOT_FOUND) {
            return true;
        }

        if (code == ERROR_ACCESS_DENIED) {
            const DWORD attrs = GetFileAttributesA(path.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
                SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
                if (DeleteFileA(path.c_str())) {
                    return true;
                }
            }
        }

        errorOut = FormatWin32Error(GetLastError());
        return false;
    }

    return true;
}

void ShellOpen(const std::string& target) {
    ShellExecuteA(nullptr, "open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

std::string QuoteCommandArg(const std::string& value) {
    std::string trimmed = value;
    while (!trimmed.empty() && (trimmed.back() == '\\' || trimmed.back() == '/')) {
        trimmed.pop_back();
    }
    std::string out = "\"";
    for (char ch : trimmed) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out += ch;
        }
    }
    out += "\"";
    return out;
}

std::vector<std::string> ParseCommandLineArgs() {
    std::vector<std::string> args;
    for (int i = 0; i < __argc; ++i) {
        args.push_back(__argv[i]);
    }
    return args;
}
