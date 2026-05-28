#include "startup.h"

#include "app.h"
#include "dialogs.h"
#include "path_util.h"
#include "paths.h"
#include "settings_store.h"
#include "util.h"

#include <vector>

namespace {

constexpr wchar_t kRunKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool SetRunEntry(const std::wstring& executablePath) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t full[MAX_PATH] {};
    if (!GetFullPathNameW(executablePath.c_str(), MAX_PATH, full, nullptr)) {
        RegCloseKey(key);
        return false;
    }

    const std::wstring quoted = L"\"" + std::wstring(full) + L"\"";
    const LSTATUS status = RegSetValueExW(
        key,
        paths::kRunValueName,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(quoted.c_str()),
        static_cast<DWORD>((quoted.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

void RemoveRunEntry() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegDeleteValueW(key, paths::kRunValueName);
        RegCloseKey(key);
    }
}

bool LaunchInstalledCopy(const std::wstring& destinationExe, const std::wstring& destinationDir, const std::wstring& sourceExe) {
    if (!FilePathExists(destinationExe)) {
        return false;
    }

    std::wstring commandLine = QuoteCommandArg(destinationExe) + L" " + paths::FormatCleanupSourceArgument(sourceExe);
    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW si {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi {};

    if (!CreateProcessW(
            destinationExe.c_str(),
            mutableCommand.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            destinationDir.c_str(),
            &si,
            &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 1000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode == STILL_ACTIVE;
}

StartupEnableResult ApplyEnable(const std::wstring& executablePath) {
    if (!SetRunEntry(executablePath)) {
        return StartupEnableResult::Failed;
    }
    SettingsSaveStartWithWindows(true);
    return StartupEnableResult::Enabled;
}

StartupEnableResult TryRelocateAndEnable(HWND owner) {
    const std::wstring sourceExe = paths::CurrentExecutablePath();
    const std::wstring destinationDir = paths::InstallDirectory();
    const std::wstring destinationExe = destinationDir + L"\\" + PathGetFileName(sourceExe);

    CreateDirectoryW(destinationDir.c_str(), nullptr);

    const std::vector<std::wstring> artifacts = paths::RelocateArtifactPaths(sourceExe);
    for (size_t i = 0; i < artifacts.size(); ++i) {
        const std::wstring& sourcePath = artifacts[i];
        const std::wstring destinationPath = destinationDir + L"\\" + PathGetFileName(sourcePath);
        std::wstring copyError;
        if (!PathCopyFileOverwrite(sourcePath, destinationPath, copyError)) {
            const std::wstring message = L"Could not copy AwakeGuard:\n" + copyError;
            DialogsShowError(owner, L"Copy failed", message.c_str());
            return StartupEnableResult::Failed;
        }
    }

    SettingsSaveStartWithWindows(true);
    if (!SetRunEntry(destinationExe)) {
        DialogsShowError(owner, L"Copy failed", L"Could not enable startup with Windows after copying.");
        return StartupEnableResult::Failed;
    }

    if (!LaunchInstalledCopy(destinationExe, destinationDir, sourceExe)) {
        DialogsShowError(owner, L"Copy failed", L"The AppData copy exited immediately or could not be started.");
        return StartupEnableResult::Failed;
    }

    PostQuitMessage(0);
    return StartupEnableResult::Relocating;
}

}  // namespace

bool StartupIsRegistered() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }
    wchar_t buffer[MAX_PATH] {};
    DWORD size = sizeof(buffer);
    const LSTATUS status = RegQueryValueExW(key, paths::kRunValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

StartupEnableResult StartupTryEnable(AppState& app, bool offerRelocate) {
    const std::wstring currentExe = paths::CurrentExecutablePath();

    if (offerRelocate && !paths::IsRunningFromInstallLocation() && paths::IsOnDesktopOrDownloads(currentExe)) {
        if (DialogsShowConfirm(
                app.settingsWindow,
                L"Copy AwakeGuard?",
                L"AwakeGuard is running from your Desktop or Downloads folder. "
                L"For reliable startup with Windows, it's best to keep a copy in your AppData folder.\n\n"
                L"Copy AwakeGuard there now?",
                L"Copy",
                L"Not now")) {
            return TryRelocateAndEnable(app.settingsWindow);
        }
    }

    const StartupEnableResult result = ApplyEnable(currentExe);
    if (result == StartupEnableResult::Failed) {
        DialogsShowError(app.settingsWindow, L"Startup failed", L"Could not enable startup with Windows.");
    }
    return result;
}

void StartupDisable() {
    RemoveRunEntry();
    SettingsSaveStartWithWindows(false);
}
