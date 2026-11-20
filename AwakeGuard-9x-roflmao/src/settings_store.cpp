#include "settings_store.h"

#include "paths.h"
#include "win32_common.h"

namespace {

// settings.json is tiny (one boolean); anything larger isn't ours.
constexpr DWORD kMaxSettingsBytes = 4096;
char g_settingsBuffer[kMaxSettingsBytes + 1];

bool ReadSettingsText(const char* path, const char*& textOut) {
    textOut = "";
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD read = 0;
    const BOOL ok = ReadFile(file, g_settingsBuffer, kMaxSettingsBytes, &read, nullptr);
    CloseHandle(file);
    if (!ok) {
        return false;
    }
    g_settingsBuffer[read] = '\0';
    textOut = g_settingsBuffer;
    return true;
}

bool Contains(const char* haystack, const char* needle) {
    const int needleLen = lstrlenA(needle);
    for (const char* p = haystack; *p; ++p) {
        int i = 0;
        while (i < needleLen && p[i] == needle[i]) {
            ++i;
        }
        if (i == needleLen) {
            return true;
        }
    }
    return false;
}

bool WriteFileText(const char* path, const char* text) {
    CreateDirectoryA(paths::DataDirectory().c_str(), nullptr);

    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = WriteFile(file, text, static_cast<DWORD>(lstrlenA(text)), &written, nullptr);
    CloseHandle(file);
    return ok != FALSE;
}

}  // namespace

bool SettingsLoadStartWithWindows() {
    const char* text = nullptr;
    if (!ReadSettingsText(paths::SettingsFilePath().c_str(), text)) {
        return false;
    }
    return Contains(text, "\"StartWithWindows\": true") || Contains(text, "\"StartWithWindows\":true");
}

void SettingsSaveStartWithWindows(bool enabled) {
    const char* json = enabled
        ? "{\r\n  \"StartWithWindows\": true\r\n}\r\n"
        : "{\r\n  \"StartWithWindows\": false\r\n}\r\n";
    WriteFileText(paths::SettingsFilePath().c_str(), json);
}
