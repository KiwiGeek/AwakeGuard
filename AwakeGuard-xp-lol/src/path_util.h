#pragma once

#include <string>

std::wstring PathGetFileName(const std::wstring& path);
bool FilePathExists(const std::wstring& path);
bool PathCopyFileOverwrite(const std::wstring& source, const std::wstring& destination, std::wstring& errorOut);
