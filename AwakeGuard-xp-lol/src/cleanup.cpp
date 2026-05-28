#include "cleanup.h"

#include <vector>

#include "dialogs.h"
#include "path_util.h"
#include "paths.h"
#include "util.h"

namespace {

bool PathsReferToSameDirectory(const std::wstring& first, const std::wstring& second) {
    wchar_t a[MAX_PATH] {};
    wchar_t b[MAX_PATH] {};
    if (!GetFullPathNameW(first.c_str(), MAX_PATH, a, nullptr) ||
        !GetFullPathNameW(second.c_str(), MAX_PATH, b, nullptr)) {
        return false;
    }
    return _wcsicmp(a, b) == 0;
}

bool InstallArtifactMatchesSource(const std::wstring& sourcePath, const std::wstring& installPath) {
    if (!FilePathExists(sourcePath) || !FilePathExists(installPath)) {
        return false;
    }

    ULONGLONG sourceSize = 0;
    ULONGLONG installSize = 0;
    if (!GetFileSizeBytes(sourcePath, sourceSize) || !GetFileSizeBytes(installPath, installSize)) {
        return false;
    }

    return sourceSize > 0 && sourceSize == installSize;
}

bool CleanupMatchingArtifacts(const std::wstring& sourceExecutablePath, const std::wstring& installDirectory, std::wstring& errorOut) {
    errorOut.clear();

    const std::vector<std::wstring> artifacts = paths::RelocateArtifactPaths(sourceExecutablePath);
    for (size_t i = 0; i < artifacts.size(); ++i) {
        const std::wstring& sourcePath = artifacts[i];
        if (!FilePathExists(sourcePath)) {
            continue;
        }

        const std::wstring installPath = installDirectory + L"\\" + PathGetFileName(sourcePath);
        if (!InstallArtifactMatchesSource(sourcePath, installPath)) {
            errorOut = L"Could not verify the AppData copy of\n" + PathGetFileName(sourcePath) +
                       L" before deleting the original.";
            return false;
        }

        std::wstring deleteError;
        if (!DeletePathAllowReadonly(sourcePath, deleteError)) {
            errorOut = L"Could not delete:\n" + sourcePath + L"\n\n" + deleteError;
            return false;
        }
    }

    return true;
}

std::wstring ParentDirectory(const std::wstring& filePath) {
    const size_t pos = filePath.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L"";
    }
    return filePath.substr(0, pos);
}

}  // namespace

void CleanupOfferPrompt(HWND owner, const std::wstring& sourceExecutablePath) {
    if (!paths::IsRunningFromInstallLocation()) {
        return;
    }

    wchar_t full[MAX_PATH] {};
    if (!GetFullPathNameW(sourceExecutablePath.c_str(), MAX_PATH, full, nullptr)) {
        return;
    }

    const std::wstring normalizedExecutable = full;
    if (!FilePathExists(normalizedExecutable) || !paths::IsOnDesktopOrDownloads(normalizedExecutable)) {
        return;
    }

    const std::wstring sourceDirectory = ParentDirectory(normalizedExecutable);
    const std::wstring installDirectory = paths::InstallDirectory();
    if (PathsReferToSameDirectory(sourceDirectory, installDirectory)) {
        return;
    }

    const std::wstring message =
        L"AwakeGuard was copied from:\n" + normalizedExecutable +
        L"\n\nDelete the original executable and its .pdb (if present)?";

    if (!DialogsShowConfirm(owner, L"Delete original copy?", message.c_str(), L"Delete", L"Keep", true)) {
        return;
    }

    std::wstring errorOut;
    if (!CleanupMatchingArtifacts(normalizedExecutable, installDirectory, errorOut)) {
        DialogsShowError(owner, L"Cleanup failed", errorOut.c_str());
    }
}
