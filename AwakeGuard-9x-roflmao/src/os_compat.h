#pragma once

#include "win32_common.h"

void OsCompatInit();
bool OsHasSetThreadExecutionState();
bool OsSupportsTrayBalloons();
bool OsIsWindows9x();
DWORD OsSetThreadExecutionState(DWORD flags);
UINT OsNotifyIconDataSize(bool includeBalloonFields);

// SHGetSpecialFolderPathA (shell32); nullptr if unavailable.
typedef BOOL(WINAPI* SHGetSpecialFolderPathFn)(HWND, LPSTR, int, BOOL);

bool OsGetSpecialFolderPath(int folderId, bool create, char* path, UINT pathChars);
bool OsGetUserDataRoot(char* path, UINT pathChars);
