#include "cleanup.h"

#include "dialogs.h"
#include "paths.h"
#include "util.h"

#include <filesystem>

namespace fs = std::filesystem;

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
    if (!fs::exists(sourcePath) || !fs::exists(installPath)) {
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

    for (const auto& sourcePath : paths::RelocateArtifactPaths(sourceExecutablePath)) {
        if (!fs::exists(sourcePath)) {
            continue;
        }

        const std::wstring installPath = installDirectory + L"\\" + fs::path(sourcePath).filename().wstring();
        if (!InstallArtifactMatchesSource(sourcePath, installPath)) {
            errorOut =
                L"Could not verify the AppData copy of\n" + fs::path(sourcePath).filename().wstring() +
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
    if (!fs::exists(normalizedExecutable) || !paths::IsOnDesktopOrDownloads(normalizedExecutable)) {
        return;
    }

    const std::wstring sourceDirectory = fs::path(normalizedExecutable).parent_path().wstring();
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
