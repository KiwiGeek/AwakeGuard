#pragma once

#include "str.h"
#include "win32_common.h"

constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT WM_SESSION_CHANGED = WM_APP + 2;

constexpr UINT_PTR IDT_REFRESH = 1;
constexpr UINT_PTR IDT_EXPIRY = 2;

constexpr UINT REFRESH_MS = 60 * 1000;
constexpr UINT EXPIRY_MS = 15 * 1000;

TextStr FormatWin32Error(DWORD code);
PathStr GetExePath();
bool FilesIdentical(const char* a, const char* b);
bool DeletePathAllowReadonly(const char* path, TextStr& errorOut);
void ShellOpen(const char* target);
TextStr QuoteCommandArg(const char* value);

// "--trace" on the command line: show a message box before each startup step,
// so a hard crash on a museum OS can be pinned to the last box shown.
void TraceInit();
void Trace(const char* step);
