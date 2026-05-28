#include "paths.h"

#include "path_util.h"
#include "util.h"

#include <shlobj.h>
#include <vector>

namespace paths {

namespace {

bool IsUnderDirectory(const std::wstring& path, const std::wstring& parent) {
    wchar_t fullPath[MAX_PATH] {};
    wchar_t fullParent[MAX_PATH] {};
    if (!GetFullPathNameW(path.c_str(), MAX_PATH, fullPath, nullptr) ||
        !GetFullPathNameW(parent.c_str(), MAX_PATH, fullParent, nullptr)) {
        return false;
    }

    std::wstring normalizedPath = fullPath;
    std::wstring normalizedParent = fullParent;
    if (!normalizedPath.empty() && normalizedPath.back() != L'\\') {
        normalizedPath += L'\\';
    }
    if (!normalizedParent.empty() && normalizedParent.back() != L'\\') {
        normalizedParent += L'\\';
    }
    return _wcsnicmp(normalizedPath.c_str(), normalizedParent.c_str(), normalizedParent.size()) == 0;
}

std::vector<std::wstring> TransientInstallRoots() {
    std::vector<std::wstring> roots;
    wchar_t buffer[MAX_PATH] {};

    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        roots.push_back(buffer);
    }
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        roots.push_back(buffer);
    }
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        std::wstring downloads = buffer;
        if (!downloads.empty() && downloads.back() != L'\\') {
            downloads += L'\\';
        }
        downloads += L"Downloads";
        roots.push_back(downloads);
    }
    return roots;
}

}  // namespace

std::wstring LocalAppDataDirectory() {
    wchar_t buffer[MAX_PATH] {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        return L"";
    }
    return buffer;
}

std::wstring DataDirectory() {
    std::wstring dir = LocalAppDataDirectory();
    if (dir.empty()) {
        return L"";
    }
    if (dir.back() != L'\\') {
        dir += L'\\';
    }
    dir += kAppFolderName;
    return dir;
}

std::wstring SettingsFilePath() {
    std::wstring dir = DataDirectory();
    if (dir.empty()) {
        return L"";
    }
    if (dir.back() != L'\\') {
        dir += L'\\';
    }
    dir += L"settings.json";
    return dir;
}

std::wstring InstallDirectory() {
    return DataDirectory();
}

std::wstring CurrentExecutablePath() {
    return GetExePath();
}

bool IsRunningFromInstallLocation() {
    const std::wstring current = CurrentExecutablePath();
    const std::wstring install = InstallDirectory();
    if (current.empty() || install.empty()) {
        return false;
    }
    return IsUnderDirectory(current, install);
}

bool IsTransientDirectory(const std::wstring& directory) {
    if (directory.empty()) {
        return false;
    }
    wchar_t full[MAX_PATH] {};
    if (!GetFullPathNameW(directory.c_str(), MAX_PATH, full, nullptr)) {
        return false;
    }
    const std::vector<std::wstring> roots = TransientInstallRoots();
    for (size_t i = 0; i < roots.size(); ++i) {
        if (IsUnderDirectory(full, roots[i])) {
            return true;
        }
    }
    return false;
}

bool IsOnDesktopOrDownloads(const std::wstring& filePath) {
    std::wstring path = filePath.empty() ? CurrentExecutablePath() : filePath;
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return false;
    }
    return IsTransientDirectory(path.substr(0, pos));
}

std::vector<std::wstring> RelocateArtifactPaths(const std::wstring& sourceExecutablePath) {
    std::vector<std::wstring> artifacts;

    wchar_t full[MAX_PATH] {};
    if (!GetFullPathNameW(sourceExecutablePath.c_str(), MAX_PATH, full, nullptr)) {
        return artifacts;
    }

    artifacts.push_back(full);

    std::wstring pdbPath = full;
    const size_t dot = pdbPath.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        pdbPath = pdbPath.substr(0, dot);
    }
    pdbPath += L".pdb";
    if (FilePathExists(pdbPath)) {
        artifacts.push_back(pdbPath);
    }

    return artifacts;
}

std::wstring TryGetCleanupSourceExecutable(const std::vector<std::wstring>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (_wcsicmp(args[i].c_str(), kCleanupFlag) == 0 && i + 1 < args.size()) {
            wchar_t full[MAX_PATH] {};
            if (GetFullPathNameW(args[i + 1].c_str(), MAX_PATH, full, nullptr)) {
                return full;
            }
        }
        const std::wstring prefix = std::wstring(kCleanupFlag) + L"=";
        if (_wcsnicmp(args[i].c_str(), prefix.c_str(), prefix.size()) == 0) {
            wchar_t full[MAX_PATH] {};
            if (GetFullPathNameW(args[i].substr(prefix.size()).c_str(), MAX_PATH, full, nullptr)) {
                return full;
            }
        }
    }
    return L"";
}

std::wstring FormatCleanupSourceArgument(const std::wstring& sourceExecutablePath) {
    return std::wstring(kCleanupFlag) + L" " + QuoteCommandArg(sourceExecutablePath);
}

}  // namespace paths
