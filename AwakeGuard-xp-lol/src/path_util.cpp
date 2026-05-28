#include "path_util.h"

#include <shlwapi.h>

bool FilePathExists(const std::wstring& path) {
    return PathFileExistsW(path.c_str()) != FALSE;
}

std::wstring PathGetFileName(const std::wstring& path) {
    const wchar_t* name = PathFindFileNameW(path.c_str());
    return name ? std::wstring(name) : std::wstring();
}

bool PathCopyFileOverwrite(const std::wstring& source, const std::wstring& destination, std::wstring& errorOut) {
    errorOut.clear();
    if (!FilePathExists(source)) {
        errorOut = L"Source file not found.";
        return false;
    }

    if (!CopyFileW(source.c_str(), destination.c_str(), FALSE)) {
        errorOut = L"CopyFile failed.";
        return false;
    }

    return true;
}
