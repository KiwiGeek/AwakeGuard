#include "settings_store.h"

#include "paths.h"
#include "win32_common.h"

#include <fstream>
#include <string>

namespace {

bool ReadFileText(const std::wstring& path, std::string& text) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool WriteFileText(const std::wstring& path, const std::string& text) {
    const std::wstring dir = paths::DataDirectory();
    CreateDirectoryW(dir.c_str(), nullptr);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(out);
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
