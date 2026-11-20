#include "path_util.h"

bool FilePathExists(const char* path) {
    const DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
}

PathStr PathGetFileName(const char* path) {
    const PathStr full = path;
    const int slash = full.FindLastSlash();
    if (slash < 0) {
        return full;
    }
    return PathStr(full.c_str() + slash + 1);
}

PathStr PathGetParent(const char* path) {
    PathStr parent = path;
    const int slash = parent.FindLastSlash();
    if (slash < 0) {
        return PathStr();
    }
    parent.Truncate(slash);
    return parent;
}

const char* PathCopyFileOverwrite(const char* source, const char* destination) {
    if (!FilePathExists(source)) {
        return "Source file not found.";
    }
    if (!CopyFileA(source, destination, FALSE)) {
        return "CopyFile failed.";
    }
    return nullptr;
}
