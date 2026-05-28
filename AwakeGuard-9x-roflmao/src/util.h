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

std::string FormatWin32Error(DWORD code);
std::string GetExePath();
std::string GetExeDirectory();
bool FileSha256Hex(const std::string& path, std::string& hexOut);
bool FilesIdentical(const std::string& a, const std::string& b);
bool GetFileSizeBytes(const std::string& path, ULONGLONG& sizeOut);
bool DeletePathAllowReadonly(const std::string& path, std::string& errorOut);
void ShellOpen(const std::string& target);
std::string QuoteCommandArg(const std::string& value);
std::vector<std::string> ParseCommandLineArgs();
