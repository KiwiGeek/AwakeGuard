#include "os_compat.h"

#include <stdio.h>
#include <string.h>

namespace {

typedef DWORD(WINAPI* SetThreadExecutionStateFn)(DWORD);

SetThreadExecutionStateFn g_setThreadExecutionState = nullptr;
SHGetSpecialFolderPathFn g_shGetSpecialFolderPath = nullptr;
bool g_supportsTrayBalloons = false;
bool g_isWindows9x = false;
bool g_initialized = false;

}  // namespace

void OsCompatInit() {
    if (g_initialized) {
        return;
    }
    g_initialized = true;

    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) {
        kernel32 = LoadLibraryA("kernel32.dll");
    }
    if (kernel32) {
        g_setThreadExecutionState = reinterpret_cast<SetThreadExecutionStateFn>(
            GetProcAddress(kernel32, "SetThreadExecutionState"));
    }

    HMODULE shell32 = GetModuleHandleA("shell32.dll");
    if (!shell32) {
        shell32 = LoadLibraryA("shell32.dll");
    }
    if (shell32) {
        g_shGetSpecialFolderPath = reinterpret_cast<SHGetSpecialFolderPathFn>(
            GetProcAddress(shell32, "SHGetSpecialFolderPathA"));
    }

    OSVERSIONINFOA version {};
    version.dwOSVersionInfoSize = sizeof(version);
    if (GetVersionExA(&version)) {
        g_isWindows9x = version.dwMajorVersion < 5;
        g_supportsTrayBalloons = version.dwMajorVersion >= 5;
    }
}

bool OsHasSetThreadExecutionState() {
    OsCompatInit();
    return g_setThreadExecutionState != nullptr;
}

bool OsSupportsTrayBalloons() {
    OsCompatInit();
    return g_supportsTrayBalloons;
}

bool OsIsWindows9x() {
    OsCompatInit();
    return g_isWindows9x;
}

DWORD OsSetThreadExecutionState(DWORD flags) {
    OsCompatInit();
    if (!g_setThreadExecutionState) {
        return 0;
    }
    return g_setThreadExecutionState(flags);
}

UINT OsNotifyIconDataSize(bool includeBalloonFields) {
    OsCompatInit();
    if (OsIsWindows9x()) {
        return static_cast<UINT>(FIELD_OFFSET(NOTIFYICONDATAA, szTip) + sizeof(((NOTIFYICONDATAA*)0)->szTip));
    }
    if (includeBalloonFields) {
        return sizeof(NOTIFYICONDATAA);
    }
    return static_cast<UINT>(FIELD_OFFSET(NOTIFYICONDATAA, szInfo));
}

bool OsGetSpecialFolderPath(int folderId, bool create, char* path, UINT pathChars) {
    OsCompatInit();
    if (!g_shGetSpecialFolderPath || !path || pathChars < MAX_PATH) {
        return false;
    }
    const int flags = create ? (folderId | CSIDL_FLAG_CREATE) : folderId;
    return g_shGetSpecialFolderPath(nullptr, path, flags, FALSE) != FALSE;
}

bool OsGetUserDataRoot(char* path, UINT pathChars) {
    if (!path || pathChars < MAX_PATH) {
        return false;
    }
    path[0] = '\0';

    if (OsGetSpecialFolderPath(CSIDL_APPDATA, true, path, pathChars)) {
        return true;
    }

    char windowsDir[MAX_PATH] {};
    char userName[256] {};
    DWORD userLen = static_cast<DWORD>(sizeof(userName));
    if (!GetWindowsDirectoryA(windowsDir, MAX_PATH) || !GetUserNameA(userName, &userLen)) {
        return false;
    }

    // Win95 multi-user: C:\Windows\Profiles\<user>
    snprintf(path, pathChars, "%s\\Profiles\\%s", windowsDir, userName);
    path[pathChars - 1] = '\0';

    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(path, nullptr)) {
            return false;
        }
    }

    return true;
}
