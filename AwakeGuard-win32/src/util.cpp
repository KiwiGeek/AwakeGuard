#include "util.h"

#include <bcrypt.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <sstream>
#include <fstream>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shlwapi.lib")

std::wstring FormatWin32Error(DWORD code) {
    wchar_t* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD len = FormatMessageW(flags, nullptr, code, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring message = len ? std::wstring(buffer, len) : L"Unknown error";
    if (buffer) {
        LocalFree(buffer);
    }
    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) {
        message.pop_back();
    }
    return message;
}

std::wstring GetExePath() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (len == 0) {
            return L"";
        }
        if (len < path.size() - 1) {
            path.resize(len);
            return path;
        }
        path.resize(path.size() * 2);
    }
}

std::wstring GetExeDirectory() {
    const std::wstring exe = GetExePath();
    const auto pos = exe.find_last_of(L"\\/");
    return pos == std::wstring::npos ? L"" : exe.substr(0, pos);
}

bool FileSha256Hex(const std::wstring& path, std::wstring& hexOut) {
    hexOut.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD hashObjectSize = 0;
    DWORD hashLength = 0;
    DWORD cbData = 0;
    bool ok = false;

    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        CloseHandle(file);
        return false;
    }

    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&hashObjectSize), sizeof(hashObjectSize), &cbData, 0) != 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &cbData, 0) != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        CloseHandle(file);
        return false;
    }

    std::vector<UCHAR> hashObject(hashObjectSize);
    std::vector<UCHAR> hashBytes(hashLength);

    if (BCryptCreateHash(alg, &hash, hashObject.data(), hashObjectSize, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        CloseHandle(file);
        return false;
    }

    std::vector<BYTE> buffer(64 * 1024);
    DWORD read = 0;
    while (ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) && read > 0) {
        if (BCryptHashData(hash, buffer.data(), read, 0) != 0) {
            goto cleanup;
        }
    }

    if (BCryptFinishHash(hash, hashBytes.data(), hashLength, 0) == 0) {
        static const wchar_t* digits = L"0123456789ABCDEF";
        hexOut.reserve(hashLength * 2);
        for (DWORD i = 0; i < hashLength; ++i) {
            hexOut.push_back(digits[(hashBytes[i] >> 4) & 0xF]);
            hexOut.push_back(digits[hashBytes[i] & 0xF]);
        }
        ok = true;
    }

cleanup:
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(file);
    return ok;
}

bool FilesIdentical(const std::wstring& a, const std::wstring& b) {
    WIN32_FILE_ATTRIBUTE_DATA fa {};
    WIN32_FILE_ATTRIBUTE_DATA fb {};
    if (!GetFileAttributesExW(a.c_str(), GetFileExInfoStandard, &fa) ||
        !GetFileAttributesExW(b.c_str(), GetFileExInfoStandard, &fb)) {
        return false;
    }

    if (fa.nFileSizeLow != fb.nFileSizeLow || fa.nFileSizeHigh != fb.nFileSizeHigh) {
        return false;
    }

    std::wstring ha;
    std::wstring hb;
    return FileSha256Hex(a, ha) && FileSha256Hex(b, hb) && _wcsicmp(ha.c_str(), hb.c_str()) == 0;
}

bool GetFileSizeBytes(const std::wstring& path, ULONGLONG& sizeOut) {
    WIN32_FILE_ATTRIBUTE_DATA info {};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info)) {
        return false;
    }

    sizeOut = (static_cast<ULONGLONG>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    return true;
}

bool DeletePathAllowReadonly(const std::wstring& path, std::wstring& errorOut) {
    errorOut.clear();
    if (!DeleteFileW(path.c_str())) {
        const DWORD code = GetLastError();
        if (code == ERROR_FILE_NOT_FOUND) {
            return true;
        }

        if (code == ERROR_ACCESS_DENIED) {
            const DWORD attrs = GetFileAttributesW(path.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
                SetFileAttributesW(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
                if (DeleteFileW(path.c_str())) {
                    return true;
                }
            }
        }

        errorOut = FormatWin32Error(GetLastError());
        return false;
    }

    return true;
}

void ShellOpen(const std::wstring& target) {
    ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

std::wstring QuoteCommandArg(const std::wstring& value) {
    std::wstring trimmed = value;
    while (!trimmed.empty() && (trimmed.back() == L'\\' || trimmed.back() == L'/')) {
        trimmed.pop_back();
    }
    std::wstring out = L"\"";
    for (wchar_t ch : trimmed) {
        if (ch == L'"') {
            out += L"\\\"";
        } else {
            out += ch;
        }
    }
    out += L"\"";
    return out;
}

std::vector<std::wstring> ParseCommandLineArgs() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args;
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        LocalFree(argv);
    }
    return args;
}
