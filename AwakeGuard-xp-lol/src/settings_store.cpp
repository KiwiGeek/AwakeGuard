#include "settings_store.h"

#include "paths.h"
#include "win32_common.h"

#include <string>
#include <vector>

namespace {

bool ReadFileText(const std::wstring& path, std::string& text) {
    text.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    const DWORD size = GetFileSize(file, nullptr);
    if (size == INVALID_FILE_SIZE || size == 0) {
        CloseHandle(file);
        return size == 0;
    }

    std::vector<char> buffer(size);
    DWORD read = 0;
    const BOOL ok = ReadFile(file, buffer.data(), size, &read, nullptr);
    CloseHandle(file);
    if (!ok) {
        return false;
    }

    text.assign(buffer.begin(), buffer.begin() + read);
    return true;
}

bool WriteFileText(const std::wstring& path, const std::string& text) {
    const std::wstring dir = paths::DataDirectory();
    CreateDirectoryW(dir.c_str(), nullptr);

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = !text.empty()
        ? WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr)
        : WriteFile(file, "", 0, &written, nullptr);
    CloseHandle(file);
    return ok != FALSE;
}

}  // namespace

bool SettingsLoadStartWithWindows() {
    std::string text;
    if (!ReadFileText(paths::SettingsFilePath(), text)) {
        return false;
    }
    return text.find("\"StartWithWindows\": true") != std::string::npos ||
           text.find("\"StartWithWindows\":true") != std::string::npos;
}

void SettingsSaveStartWithWindows(bool enabled) {
    const std::string json = enabled
        ? "{\r\n  \"StartWithWindows\": true\r\n}\r\n"
        : "{\r\n  \"StartWithWindows\": false\r\n}\r\n";
    WriteFileText(paths::SettingsFilePath(), json);
}
