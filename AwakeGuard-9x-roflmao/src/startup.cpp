#include "startup.h"

#include "app.h"
#include "dialogs.h"
#include "path_util.h"
#include "paths.h"
#include "settings_store.h"
#include "util.h"

namespace {

constexpr char kRunKeyPath[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool SetRunEntry(const char* executablePath) {
    HKEY key = nullptr;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, kRunKeyPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    PathStr full;
    if (!GetFullPathNameA(executablePath, MAX_PATH, full.Buffer(), nullptr)) {
        RegCloseKey(key);
        return false;
    }
    full.SyncLength();

    TextStr quoted = "\"";
    quoted += full;
    quoted += "\"";
    const LSTATUS status = RegSetValueExA(
        key,
        paths::kRunValueName,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(quoted.c_str()),
        static_cast<DWORD>(quoted.size() + 1));
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

bool LaunchInstalledCopy(const char* destinationExe, const char* destinationDir, const char* sourceExe) {
    if (!FilePathExists(destinationExe)) {
        return false;
    }

    CommandStr commandLine;
    commandLine += QuoteCommandArg(destinationExe);
    commandLine += " ";
    commandLine += paths::FormatCleanupSourceArgument(sourceExe);

    STARTUPINFOA si {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi {};

    // CreateProcessA may write to the command-line buffer, so pass our own copy.
    if (!CreateProcessA(
            destinationExe,
            commandLine.Buffer(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            destinationDir,
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

StartupEnableResult ApplyEnable(const char* executablePath) {
    if (!SetRunEntry(executablePath)) {
        return StartupEnableResult::Failed;
    }
    SettingsSaveStartWithWindows(true);
    return StartupEnableResult::Enabled;
}

StartupEnableResult TryRelocateAndEnable(HWND owner) {
    const PathStr sourceExe = paths::CurrentExecutablePath();
    const PathStr destinationDir = paths::InstallDirectory();
    PathStr destinationExe = destinationDir;
    destinationExe += "\\";
    destinationExe += PathGetFileName(sourceExe.c_str());

    CreateDirectoryA(destinationDir.c_str(), nullptr);

    const paths::ArtifactList artifacts = paths::RelocateArtifactPaths(sourceExe.c_str());
    for (int i = 0; i < artifacts.count; ++i) {
        const PathStr& sourcePath = artifacts.items[i];
        PathStr destinationPath = destinationDir;
        destinationPath += "\\";
        destinationPath += PathGetFileName(sourcePath.c_str());
        const char* copyError = PathCopyFileOverwrite(sourcePath.c_str(), destinationPath.c_str());
        if (copyError) {
            TextStr message = "Could not copy AwakeGuard:\n";
            message += copyError;
            DialogsShowError(owner, "Copy failed", message.c_str());
            return StartupEnableResult::Failed;
        }
    }

    SettingsSaveStartWithWindows(true);
    if (!SetRunEntry(destinationExe.c_str())) {
        DialogsShowError(owner, "Copy failed", "Could not enable startup with Windows after copying.");
        return StartupEnableResult::Failed;
    }

    if (!LaunchInstalledCopy(destinationExe.c_str(), destinationDir.c_str(), sourceExe.c_str())) {
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
    const PathStr currentExe = paths::CurrentExecutablePath();

    if (offerRelocate && !paths::IsRunningFromInstallLocation() && paths::IsOnDesktopOrDownloads(currentExe.c_str())) {
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

    const StartupEnableResult result = ApplyEnable(currentExe.c_str());
    if (result == StartupEnableResult::Failed) {
        DialogsShowError(app.settingsWindow, "Startup failed", "Could not enable startup with Windows.");
    }
    return result;
}

void StartupDisable() {
    RemoveRunEntry();
    SettingsSaveStartWithWindows(false);
}
