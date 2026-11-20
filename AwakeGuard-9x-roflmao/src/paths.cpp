#include "paths.h"

#include "os_compat.h"
#include "path_util.h"
#include "util.h"

namespace paths {

namespace {

bool IsUnderDirectory(const char* path, const char* parent) {
    PathStr fullPath;
    PathStr fullParent;
    if (!GetFullPathNameA(path, MAX_PATH, fullPath.Buffer(), nullptr) ||
        !GetFullPathNameA(parent, MAX_PATH, fullParent.Buffer(), nullptr)) {
        return false;
    }
    fullPath.SyncLength();
    fullParent.SyncLength();
    fullPath.EnsureTrailingBackslash();
    fullParent.EnsureTrailingBackslash();
    return StrStartsWithI(fullPath.c_str(), fullParent.c_str());
}

struct RootList {
    PathStr items[2];
    int count;
};

RootList TransientInstallRoots() {
    RootList roots;
    roots.count = 0;

    PathStr desktop;
    if (OsGetSpecialFolderPath(CSIDL_DESKTOP, false, desktop.Buffer(), MAX_PATH)) {
        desktop.SyncLength();
        roots.items[roots.count++] = desktop;
    }

    PathStr downloads;
    if (OsGetSpecialFolderPath(CSIDL_PROFILE, false, downloads.Buffer(), MAX_PATH)) {
        downloads.SyncLength();
        downloads.EnsureTrailingBackslash();
        downloads += "Downloads";
        roots.items[roots.count++] = downloads;
    }
    return roots;
}

}  // namespace

PathStr UserDataRootDirectory() {
    PathStr root;
    if (OsGetSpecialFolderPath(CSIDL_LOCAL_APPDATA, true, root.Buffer(), MAX_PATH) ||
        OsGetUserDataRoot(root.Buffer(), MAX_PATH)) {
        root.SyncLength();
        return root;
    }
    return PathStr();
}

PathStr DataDirectory() {
    PathStr dir = UserDataRootDirectory();
    if (dir.empty()) {
        return dir;
    }
    dir.EnsureTrailingBackslash();
    dir += kAppFolderName;
    return dir;
}

PathStr SettingsFilePath() {
    PathStr dir = DataDirectory();
    if (dir.empty()) {
        return dir;
    }
    dir.EnsureTrailingBackslash();
    dir += "settings.json";
    return dir;
}

PathStr InstallDirectory() {
    return DataDirectory();
}

PathStr CurrentExecutablePath() {
    return GetExePath();
}

bool IsRunningFromInstallLocation() {
    const PathStr current = CurrentExecutablePath();
    const PathStr install = InstallDirectory();
    if (current.empty() || install.empty()) {
        return false;
    }
    return IsUnderDirectory(current.c_str(), install.c_str());
}

bool IsTransientDirectory(const char* directory) {
    if (!directory || !directory[0]) {
        return false;
    }
    PathStr full;
    if (!GetFullPathNameA(directory, MAX_PATH, full.Buffer(), nullptr)) {
        return false;
    }
    full.SyncLength();
    const RootList roots = TransientInstallRoots();
    for (int i = 0; i < roots.count; ++i) {
        if (IsUnderDirectory(full.c_str(), roots.items[i].c_str())) {
            return true;
        }
    }
    return false;
}

bool IsOnDesktopOrDownloads(const char* filePath) {
    const PathStr path = (filePath && filePath[0]) ? PathStr(filePath) : CurrentExecutablePath();
    if (path.FindLastSlash() < 0) {
        return false;
    }
    return IsTransientDirectory(PathGetParent(path.c_str()).c_str());
}

ArtifactList RelocateArtifactPaths(const char* sourceExecutablePath) {
    ArtifactList artifacts;
    artifacts.count = 0;

    PathStr full;
    if (!GetFullPathNameA(sourceExecutablePath, MAX_PATH, full.Buffer(), nullptr)) {
        return artifacts;
    }
    full.SyncLength();
    artifacts.items[artifacts.count++] = full;

    PathStr pdbPath = full;
    const int dot = pdbPath.FindLast('.');
    if (dot > pdbPath.FindLastSlash()) {
        pdbPath.Truncate(dot);
    }
    pdbPath += ".pdb";
    if (FilePathExists(pdbPath.c_str())) {
        artifacts.items[artifacts.count++] = pdbPath;
    }

    return artifacts;
}

PathStr TryGetCleanupSourceExecutable() {
    TextStr prefix = kCleanupFlag;
    prefix += "=";

    const char* cursor = GetCommandLineA();
    PathStr arg;
    CmdLineNextArg(cursor, arg);  // skip argv[0]
    while (CmdLineNextArg(cursor, arg)) {
        const char* value = nullptr;
        PathStr next;
        if (StrEqualsI(arg.c_str(), kCleanupFlag)) {
            if (!CmdLineNextArg(cursor, next)) {
                break;
            }
            value = next.c_str();
        } else if (StrStartsWithI(arg.c_str(), prefix.c_str())) {
            value = arg.c_str() + prefix.size();
        }
        if (value) {
            PathStr full;
            if (GetFullPathNameA(value, MAX_PATH, full.Buffer(), nullptr)) {
                full.SyncLength();
                return full;
            }
        }
    }
    return PathStr();
}

TextStr FormatCleanupSourceArgument(const char* sourceExecutablePath) {
    TextStr out = kCleanupFlag;
    out += " ";
    out += QuoteCommandArg(sourceExecutablePath);
    return out;
}

}  // namespace paths
