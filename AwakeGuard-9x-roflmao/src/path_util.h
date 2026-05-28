#pragma once

#include <string>

std::string PathGetFileName(const std::string& path);
bool FilePathExists(const std::string& path);
bool PathCopyFileOverwrite(const std::string& source, const std::string& destination, std::string& errorOut);
