#pragma once

// Fixed-capacity strings for the 9x build. The 9x exe links no C/C++ runtime
// (MSYS2's prebuilt libstdc++/CRT need a Pentium 4), so this replaces std::string.
// Appends truncate silently at capacity; every buffer is always NUL-terminated.

#include "win32_common.h"

template <int N>
class FixedStr {
public:
    FixedStr() : len_(0) { buf_[0] = '\0'; }
    FixedStr(const char* s) : len_(0) {  // NOLINT: implicit by design, like std::string
        buf_[0] = '\0';
        Append(s);
    }

    FixedStr& operator=(const char* s) {
        Clear();
        return Append(s);
    }

    FixedStr& Append(const char* s) { return Append(s, s ? lstrlenA(s) : 0); }

    FixedStr& Append(const char* s, int n) {
        for (int i = 0; i < n && len_ < N - 1; ++i) {
            buf_[len_++] = s[i];
        }
        buf_[len_] = '\0';
        return *this;
    }

    template <int M>
    FixedStr& Append(const FixedStr<M>& other) { return Append(other.c_str(), other.size()); }

    FixedStr& AppendChar(char ch) { return Append(&ch, 1); }

    FixedStr& operator+=(const char* s) { return Append(s); }
    FixedStr& operator+=(char ch) { return AppendChar(ch); }
    template <int M>
    FixedStr& operator+=(const FixedStr<M>& other) { return Append(other); }

    // Ensures a trailing backslash (no-op on empty strings).
    FixedStr& EnsureTrailingBackslash() {
        if (len_ > 0 && buf_[len_ - 1] != '\\') {
            AppendChar('\\');
        }
        return *this;
    }

    void Clear() {
        len_ = 0;
        buf_[0] = '\0';
    }

    void Truncate(int n) {
        if (n >= 0 && n < len_) {
            len_ = n;
            buf_[len_] = '\0';
        }
    }

    void PopBack() { Truncate(len_ - 1); }

    // Index of the last '\\' or '/', or -1.
    int FindLastSlash() const {
        for (int i = len_ - 1; i >= 0; --i) {
            if (buf_[i] == '\\' || buf_[i] == '/') {
                return i;
            }
        }
        return -1;
    }

    int FindLast(char ch) const {
        for (int i = len_ - 1; i >= 0; --i) {
            if (buf_[i] == ch) {
                return i;
            }
        }
        return -1;
    }

    // For APIs that write into the buffer; call SyncLength() afterwards.
    char* Buffer() { return buf_; }
    static int Capacity() { return N; }
    void SyncLength() {
        buf_[N - 1] = '\0';
        len_ = lstrlenA(buf_);
    }

    const char* c_str() const { return buf_; }
    int size() const { return len_; }
    bool empty() const { return len_ == 0; }
    char back() const { return len_ ? buf_[len_ - 1] : '\0'; }

private:
    char buf_[N];
    int len_;
};

// Sizes keep every stack frame under 4 KB: a larger frame makes GCC call
// __chkstk_ms from libgcc, which this build does not link.
typedef FixedStr<MAX_PATH> PathStr;
typedef FixedStr<128> StatusStr;  // tray/status lines
typedef FixedStr<512> TextStr;    // messages; one or two paths plus prose
typedef FixedStr<1024> CommandStr;  // command line with two quoted paths

// Case-insensitive (locale-aware via CharUpperBuffA) prefix test.
bool StrStartsWithI(const char* s, const char* prefix);
// Case-insensitive equality (lstrcmpiA).
bool StrEqualsI(const char* a, const char* b);

// Iterates Windows command-line arguments (MS C runtime quoting rules).
// Returns false when there are no more arguments.
bool CmdLineNextArg(const char*& cursor, PathStr& out);

// 64-bit / 32-bit division via one DIV instruction (no libgcc). Saturates
// to 0xFFFFFFFF if the quotient would not fit in 32 bits.
DWORD UDiv64To32(ULONGLONG n, DWORD d);
