#include "str.h"

bool StrStartsWithI(const char* s, const char* prefix) {
    const int prefixLen = lstrlenA(prefix);
    if (lstrlenA(s) < prefixLen) {
        return false;
    }
    PathStr a;
    PathStr b;
    a.Append(s, prefixLen);
    b.Append(prefix, prefixLen);
    if (a.size() != prefixLen || b.size() != prefixLen) {
        return false;  // longer than MAX_PATH; never a valid path prefix here
    }
    CharUpperBuffA(a.Buffer(), static_cast<DWORD>(prefixLen));
    CharUpperBuffA(b.Buffer(), static_cast<DWORD>(prefixLen));
    for (int i = 0; i < prefixLen; ++i) {
        if (a.c_str()[i] != b.c_str()[i]) {
            return false;
        }
    }
    return true;
}

bool StrEqualsI(const char* a, const char* b) {
    return lstrcmpiA(a, b) == 0;
}

bool CmdLineNextArg(const char*& cursor, PathStr& out) {
    out.Clear();
    const char* p = cursor;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p == '\0') {
        cursor = p;
        return false;
    }

    bool inQuotes = false;
    while (*p && (inQuotes || (*p != ' ' && *p != '\t'))) {
        int backslashes = 0;
        while (*p == '\\') {
            ++backslashes;
            ++p;
        }
        if (*p == '"') {
            for (int i = 0; i < backslashes / 2; ++i) {
                out.AppendChar('\\');
            }
            if (backslashes % 2) {
                out.AppendChar('"');  // escaped quote
            } else {
                inQuotes = !inQuotes;
            }
            ++p;
        } else {
            for (int i = 0; i < backslashes; ++i) {
                out.AppendChar('\\');
            }
            if (*p && (inQuotes || (*p != ' ' && *p != '\t'))) {
                out.AppendChar(*p);
                ++p;
            }
        }
    }
    cursor = p;
    return true;
}

DWORD UDiv64To32(ULONGLONG n, DWORD d) {
    const DWORD lo = static_cast<DWORD>(n);
    const DWORD hi = static_cast<DWORD>(n >> 32);
    if (d == 0 || hi >= d) {
        return 0xFFFFFFFFu;  // quotient would overflow (or divide by zero): DIV would fault
    }
    DWORD quotient;
    DWORD remainder;
    __asm__("divl %4" : "=a"(quotient), "=d"(remainder) : "a"(lo), "d"(hi), "rm"(d));
    (void)remainder;
    return quotient;
}
