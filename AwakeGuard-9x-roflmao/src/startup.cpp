#include "startup.h"

#include "app.h"
#include "dialogs.h"
#include "path_util.h"
#include "paths.h"
#include "settings_store.h"
#include "util.h"

#include <vector>

namespace {

constexpr char kRunKeyPath[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool SetRunEntry(const std::string& executablePath) {
    HKEY key = nullptr;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, kRunKeyPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    char full[MAX_PATH] {};
    if (!GetFullPathNameA(executablePath.c_str(), MAX_PATH, full, nullptr)) {
        RegCloseKey(key);
        return false;
    }

    const std::string quoted = "\"" + std::string(full) + "\"";
    const LSTATUS status = RegSetValueExA(
        key,
        paths::kRunValueName,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(quoted.c_str()),
        static_cast<DWORD>((quoted.size() + 1) * sizeof(char)));
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

void RemoveRunEntry() {
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegDeleteValueA(key, paths::kRunValueName);
        RegCloseKey(key);
    }
}

bool LaunchInstalledCopy(const std::string& destinationExe, const std::string& destinationDir, const std::string& sourceExe) {
    if (!FilePathExists(destinationExe)) {
        return false;
    }

    std::string commandLine = QuoteCommandArg(destinationExe) + " " + paths::FormatCleanupSourceArgument(sourceExe);
    std::vector<char> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back('\0');

    STARTUPINFOA si {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi {};

    if (!CreateProcessA(
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

StartupEnableResult ApplyEnable(const std::string& executablePath) {
    if (!SetRunEntry(executablePath)) {
        return StartupEnableResult::Failed;
    }
    SettingsSaveStartWithWindows(true);
    return StartupEnableResult::Enabled;
}

StartupEnableResult TryRelocateAndEnable(HWND owner) {
    const std::string sourceExe = paths::CurrentExecutablePath();
    const std::string destinationDir = paths::InstallDirectory();
    const std::string destinationExe = destinationDir + "\\" + PathGetFileName(sourceExe);

    CreateDirectoryA(destinationDir.c_str(), nullptr);

    const std::vector<std::string> artifacts = paths::RelocateArtifactPaths(sourceExe);
    for (size_t i = 0; i < artifacts.size(); ++i) {
        const std::string& sourcePath = artifacts[i];
        const std::string destinationPath = destinationDir + "\\" + PathGetFileName(sourcePath);
        std::string copyError;
        if (!PathCopyFileOverwrite(sourcePath, destinationPath, copyError)) {
            const std::string message = "Could not copy AwakeGuard:\n" + copyError;
            DialogsShowError(owner, "Copy failed", message.c_str());
            return StartupEnableResult::Failed;
        }
    }

    SettingsSaveStartWithWindows(true);
    if (!SetRunEntry(destinationExe)) {
        DialogsShowError(owner, "Copy failed", "Could not enable startup with Windows after copying.");
        return StartupEnableResult::Failed;
    }

    if (!LaunchInstalledCopy(destinationExe, destinationDir, sourceExe)) {
        DialogsShowError(owner, "Copy failed", "The AppData copy exited immediately or could not be started.");
        return StartupEnableResult::Failed;
    }

    PostQuitMessage(0);
    return StartupEnableResult::Relocating;
}

}  // namespace

bool StartupIsRegistered() {
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }
    char buffer[MAX_PATH] {};
    DWORD size = sizeof(buffer);
    const LSTATUS status = RegQueryValueExA(key, paths::kRunValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

StartupEnableResult StartupTryEnable(AppState& app, bool offerRelocate) {
    const std::string currentExe = paths::CurrentExecutablePath();

    if (offerRelocate && !paths::IsRunningFromInstallLocation() && paths::IsOnDesktopOrDownloads(currentExe)) {
        if (DialogsShowConfirm(
                app.settingsWindow,
                "Copy AwakeGuard?",
                "AwakeGuard is running from your Desktop or Downloads folder. "
                "For reliable startup with Windows, it's best to keep a copy in your AppData folder.\n\n"
                "Copy AwakeGuard there now?",
                "Copy",
                "Not now")) {
            return TryRelocateAndEnable(app.settingsWindow);
        }
    }

    const StartupEnableResult result = ApplyEnable(currentExe);
    if (result == StartupEnableResult::Failed) {
        DialogsShowError(app.settingsWindow, "Startup failed", "Could not enable startup with Windows.");
    }
    return result;
}

void StartupDisable() {
    RemoveRunEntry();
    SettingsSaveStartWithWindows(false);
}
