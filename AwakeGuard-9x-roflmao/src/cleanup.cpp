#include "cleanup.h"

#include <vector>

#include "dialogs.h"
#include "path_util.h"
#include "paths.h"
#include "util.h"

namespace {

bool PathsReferToSameDirectory(const std::string& first, const std::string& second) {
    char a[MAX_PATH] {};
    char b[MAX_PATH] {};
    if (!GetFullPathNameA(first.c_str(), MAX_PATH, a, nullptr) ||
        !GetFullPathNameA(second.c_str(), MAX_PATH, b, nullptr)) {
        return false;
    }
    return _stricmp(a, b) == 0;
}

bool InstallArtifactMatchesSource(const std::string& sourcePath, const std::string& installPath) {
    if (!FilePathExists(sourcePath) || !FilePathExists(installPath)) {
        return false;
    }

    return FilesIdentical(sourcePath, installPath);
}

bool CleanupMatchingArtifacts(const std::string& sourceExecutablePath, const std::string& installDirectory, std::string& errorOut) {
    errorOut.clear();

    const std::vector<std::string> artifacts = paths::RelocateArtifactPaths(sourceExecutablePath);
    for (size_t i = 0; i < artifacts.size(); ++i) {
        const std::string& sourcePath = artifacts[i];
        if (!FilePathExists(sourcePath)) {
            continue;
        }

        const std::string installPath = installDirectory + "\\" + PathGetFileName(sourcePath);
        if (!InstallArtifactMatchesSource(sourcePath, installPath)) {
            errorOut = "Could not verify the AppData copy of\n" + PathGetFileName(sourcePath) +
                       " before deleting the original.";
            return false;
        }

        std::string deleteError;
        if (!DeletePathAllowReadonly(sourcePath, deleteError)) {
            errorOut = "Could not delete:\n" + sourcePath + "\n\n" + deleteError;
            return false;
        }
    }

    return true;
}

std::string ParentDirectory(const std::string& filePath) {
    const size_t pos = filePath.find_last_of("\\/");
    if (pos == std::string::npos) {
        return "";
    }
    return filePath.substr(0, pos);
}

}  // namespace

void CleanupOfferPrompt(HWND owner, const std::string& sourceExecutablePath) {
    if (!paths::IsRunningFromInstallLocation()) {
        return;
    }

    char full[MAX_PATH] {};
    if (!GetFullPathNameA(sourceExecutablePath.c_str(), MAX_PATH, full, nullptr)) {
        return;
    }

    const std::string normalizedExecutable = full;
    if (!FilePathExists(normalizedExecutable) || !paths::IsOnDesktopOrDownloads(normalizedExecutable)) {
        return;
    }

    const std::string sourceDirectory = ParentDirectory(normalizedExecutable);
    const std::string installDirectory = paths::InstallDirectory();
    if (PathsReferToSameDirectory(sourceDirectory, installDirectory)) {
        return;
    }

    const std::string message =
        "AwakeGuard was copied from:\n" + normalizedExecutable +
        "\n\nDelete the original executable and its .pdb (if present)?";

    if (!DialogsShowConfirm(owner, "Delete original copy?", message.c_str(), "Delete", "Keep", true)) {
        return;
    }

    std::string errorOut;
    if (!CleanupMatchingArtifacts(normalizedExecutable, installDirectory, errorOut)) {
        DialogsShowError(owner, "Cleanup failed", errorOut.c_str());
    }
}
