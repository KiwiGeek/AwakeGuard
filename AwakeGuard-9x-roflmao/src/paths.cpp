#include "paths.h"

#include "os_compat.h"
#include "path_util.h"
#include "util.h"

#include <vector>

namespace paths {

namespace {

bool IsUnderDirectory(const std::string& path, const std::string& parent) {
    char fullPath[MAX_PATH] {};
    char fullParent[MAX_PATH] {};
    if (!GetFullPathNameA(path.c_str(), MAX_PATH, fullPath, nullptr) ||
        !GetFullPathNameA(parent.c_str(), MAX_PATH, fullParent, nullptr)) {
        return false;
    }

    std::string normalizedPath = fullPath;
    std::string normalizedParent = fullParent;
    if (!normalizedPath.empty() && normalizedPath.back() != '\\') {
        normalizedPath += '\\';
    }
    if (!normalizedParent.empty() && normalizedParent.back() != '\\') {
        normalizedParent += '\\';
    }
    return _strnicmp(normalizedPath.c_str(), normalizedParent.c_str(), normalizedParent.size()) == 0;
}

void AppendFolderIfFound(int folderId, std::vector<std::string>& roots) {
    char buffer[MAX_PATH] {};
    if (OsGetSpecialFolderPath(folderId, false, buffer, MAX_PATH)) {
        roots.push_back(buffer);
    }
}

std::vector<std::string> TransientInstallRoots() {
    std::vector<std::string> roots;
    AppendFolderIfFound(CSIDL_DESKTOP, roots);

    char profile[MAX_PATH] {};
    if (OsGetSpecialFolderPath(CSIDL_PROFILE, false, profile, MAX_PATH)) {
        std::string downloads = profile;
        if (!downloads.empty() && downloads.back() != '\\') {
            downloads += '\\';
        }
        downloads += "Downloads";
        roots.push_back(downloads);
    }
    return roots;
}

}  // namespace

std::string UserDataRootDirectory() {
    char buffer[MAX_PATH] {};
    if (OsGetSpecialFolderPath(CSIDL_LOCAL_APPDATA, true, buffer, MAX_PATH)) {
        return buffer;
    }
    if (OsGetUserDataRoot(buffer, MAX_PATH)) {
        return buffer;
    }
    return "";
}

std::string DataDirectory() {
    std::string dir = UserDataRootDirectory();
    if (dir.empty()) {
        return "";
    }
    if (dir.back() != '\\') {
        dir += '\\';
    }
    dir += kAppFolderName;
    return dir;
}

std::string SettingsFilePath() {
    std::string dir = DataDirectory();
    if (dir.empty()) {
        return "";
    }
    if (dir.back() != '\\') {
        dir += '\\';
    }
    dir += "settings.json";
    return dir;
}

std::string InstallDirectory() {
    return DataDirectory();
}

std::string CurrentExecutablePath() {
    return GetExePath();
}

bool IsRunningFromInstallLocation() {
    const std::string current = CurrentExecutablePath();
    const std::string install = InstallDirectory();
    if (current.empty() || install.empty()) {
        return false;
    }
    return IsUnderDirectory(current, install);
}

bool IsTransientDirectory(const std::string& directory) {
    if (directory.empty()) {
        return false;
    }
    char full[MAX_PATH] {};
    if (!GetFullPathNameA(directory.c_str(), MAX_PATH, full, nullptr)) {
        return false;
    }
    const std::vector<std::string> roots = TransientInstallRoots();
    for (size_t i = 0; i < roots.size(); ++i) {
        if (IsUnderDirectory(full, roots[i])) {
            return true;
        }
    }
    return false;
}

bool IsOnDesktopOrDownloads(const std::string& filePath) {
    std::string path = filePath.empty() ? CurrentExecutablePath() : filePath;
    const size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) {
        return false;
    }
    return IsTransientDirectory(path.substr(0, pos));
}

std::vector<std::string> RelocateArtifactPaths(const std::string& sourceExecutablePath) {
    std::vector<std::string> artifacts;

    char full[MAX_PATH] {};
    if (!GetFullPathNameA(sourceExecutablePath.c_str(), MAX_PATH, full, nullptr)) {
        return artifacts;
    }

    artifacts.push_back(full);

    std::string pdbPath = full;
    const size_t dot = pdbPath.find_last_of('.');
    if (dot != std::string::npos) {
        pdbPath = pdbPath.substr(0, dot);
    }
    pdbPath += ".pdb";
    if (FilePathExists(pdbPath)) {
        artifacts.push_back(pdbPath);
    }

    return artifacts;
}

std::string TryGetCleanupSourceExecutable(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (_stricmp(args[i].c_str(), kCleanupFlag) == 0 && i + 1 < args.size()) {
            char full[MAX_PATH] {};
            if (GetFullPathNameA(args[i + 1].c_str(), MAX_PATH, full, nullptr)) {
                return full;
            }
        }
        const std::string prefix = std::string(kCleanupFlag) + "=";
        if (_strnicmp(args[i].c_str(), prefix.c_str(), prefix.size()) == 0) {
            char full[MAX_PATH] {};
            if (GetFullPathNameA(args[i].substr(prefix.size()).c_str(), MAX_PATH, full, nullptr)) {
                return full;
            }
        }
    }
    return "";
}

std::string FormatCleanupSourceArgument(const std::string& sourceExecutablePath) {
    return std::string(kCleanupFlag) + " " + QuoteCommandArg(sourceExecutablePath);
}

}  // namespace paths
