#pragma once

#include "win32_common.h"
#include <string>
#include <vector>

constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT WM_SESSION_CHANGED = WM_APP + 2;

constexpr UINT_PTR IDT_REFRESH = 1;
constexpr UINT_PTR IDT_EXPIRY = 2;

constexpr UINT REFRESH_MS = 60 * 1000;
constexpr UINT EXPIRY_MS = 15 * 1000;

std::wstring FormatWin32Error(DWORD code);
std::wstring GetExePath();
std::wstring GetExeDirectory();
bool FileSha256Hex(const std::wstring& path, std::wstring& hexOut);
bool FilesIdentical(const std::wstring& a, const std::wstring& b);
bool GetFileSizeBytes(const std::wstring& path, ULONGLONG& sizeOut);
bool DeletePathAllowReadonly(const std::wstring& path, std::wstring& errorOut);
void ShellOpen(const std::wstring& target);
std::wstring QuoteCommandArg(const std::wstring& value);
std::vector<std::wstring> ParseCommandLineArgs();
