#include "path_util.h"

#include <windows.h>

bool FilePathExists(const std::string& path) {
    const DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

std::string PathGetFileName(const std::string& path) {
    const size_t slash = path.find_last_of("\\/");
    if (slash == std::string::npos) {
        return path;
    }
    if (slash + 1 >= path.size()) {
        return std::string();
    }
    return path.substr(slash + 1);
}

bool PathCopyFileOverwrite(const std::string& source, const std::string& destination, std::string& errorOut) {
    errorOut.clear();
    if (!FilePathExists(source)) {
        errorOut = "Source file not found.";
        return false;
    }

    if (!CopyFileA(source.c_str(), destination.c_str(), FALSE)) {
        errorOut = "CopyFile failed.";
        return false;
    }

    return true;
}
