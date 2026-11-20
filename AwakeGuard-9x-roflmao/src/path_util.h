#pragma once

#include "str.h"

PathStr PathGetFileName(const char* path);
// Everything before the last slash; empty if there is none.
PathStr PathGetParent(const char* path);
bool FilePathExists(const char* path);
// Returns nullptr on success, else a short error message.
const char* PathCopyFileOverwrite(const char* source, const char* destination);
