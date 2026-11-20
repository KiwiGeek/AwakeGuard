#include "cleanup.h"

#include "dialogs.h"
#include "path_util.h"
#include "paths.h"
#include "util.h"

namespace {

bool PathsReferToSameDirectory(const char* first, const char* second) {
    PathStr a;
    PathStr b;
    if (!GetFullPathNameA(first, MAX_PATH, a.Buffer(), nullptr) ||
        !GetFullPathNameA(second, MAX_PATH, b.Buffer(), nullptr)) {
        return false;
    }
    a.SyncLength();
    b.SyncLength();
    return StrEqualsI(a.c_str(), b.c_str());
}

bool InstallArtifactMatchesSource(const char* sourcePath, const char* installPath) {
    if (!FilePathExists(sourcePath) || !FilePathExists(installPath)) {
        return false;
    }

    return FilesIdentical(sourcePath, installPath);
}

bool CleanupMatchingArtifacts(const char* sourceExecutablePath, const char* installDirectory, TextStr& errorOut) {
    errorOut.Clear();

    const paths::ArtifactList artifacts = paths::RelocateArtifactPaths(sourceExecutablePath);
    for (int i = 0; i < artifacts.count; ++i) {
        const PathStr& sourcePath = artifacts.items[i];
        if (!FilePathExists(sourcePath.c_str())) {
            continue;
        }

        const PathStr fileName = PathGetFileName(sourcePath.c_str());
        PathStr installPath = installDirectory;
        installPath += "\\";
        installPath += fileName;
        if (!InstallArtifactMatchesSource(sourcePath.c_str(), installPath.c_str())) {
            errorOut = "Could not verify the AppData copy of\n";
            errorOut += fileName;
            errorOut += " before deleting the original.";
            return false;
        }

        TextStr deleteError;
        if (!DeletePathAllowReadonly(sourcePath.c_str(), deleteError)) {
            errorOut = "Could not delete:\n";
            errorOut += sourcePath;
            errorOut += "\n\n";
            errorOut += deleteError;
            return false;
        }
    }

    return true;
}

}  // namespace

void CleanupOfferPrompt(HWND owner, const char* sourceExecutablePath) {
    if (!paths::IsRunningFromInstallLocation()) {
        return;
    }

    PathStr normalizedExecutable;
    if (!GetFullPathNameA(sourceExecutablePath, MAX_PATH, normalizedExecutable.Buffer(), nullptr)) {
        return;
    }
    normalizedExecutable.SyncLength();

    if (!FilePathExists(normalizedExecutable.c_str()) || !paths::IsOnDesktopOrDownloads(normalizedExecutable.c_str())) {
        return;
    }

    const PathStr sourceDirectory = PathGetParent(normalizedExecutable.c_str());
    const PathStr installDirectory = paths::InstallDirectory();
    if (PathsReferToSameDirectory(sourceDirectory.c_str(), installDirectory.c_str())) {
        return;
    }

    TextStr message = "AwakeGuard was copied from:\n";
    message += normalizedExecutable;
    message += "\n\nDelete the original executable and its .pdb (if present)?";

    if (!DialogsShowConfirm(owner, "Delete original copy?", message.c_str(), "Delete", "Keep", true)) {
        return;
    }

    TextStr errorOut;
    if (!CleanupMatchingArtifacts(normalizedExecutable.c_str(), installDirectory.c_str(), errorOut)) {
        DialogsShowError(owner, "Cleanup failed", errorOut.c_str());
    }
}
