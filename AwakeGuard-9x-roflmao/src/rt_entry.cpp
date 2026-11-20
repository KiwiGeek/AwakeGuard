// Process entry point for the 9x build, replacing the MinGW CRT startup.
//
// The 9x exe is linked with -nostdlib: MSYS2's prebuilt CRT, libgcc and
// libstdc++ are compiled for Pentium 4 (LOCK CMPXCHG, CMOV, SSE) and cannot
// run on a 386/486 or under Windows 95. All code in the exe is ours, built
// with -march=i386; scripts/check-9x-exe.py verifies that in CI.

#include "win32_common.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR commandLine, int showCommand);

namespace {

// Skips argv[0] the same way the MS CRT does for WinMain's lpCmdLine.
char* SkipProgramName(char* p) {
    if (*p == '"') {
        ++p;
        while (*p && *p != '"') {
            ++p;
        }
        if (*p == '"') {
            ++p;
        }
    } else {
        while (*p && *p != ' ' && *p != '\t') {
            ++p;
        }
    }
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    return p;
}

}  // namespace

// ld's i386 PE linker script brackets all .ctors entries with these symbols
// (-1 first, 0 last). Run them so a future global with a dynamic initializer
// still works; today there are none.
typedef void (*CtorFn)();
extern "C" CtorFn __CTOR_LIST__[];

// ld's default entry point for a GUI-subsystem i386 PE image is _WinMainCRTStartup.
extern "C" void WinMainCRTStartup() {
    int count = 0;
    while (__CTOR_LIST__[count + 1]) {
        ++count;
    }
    for (int i = count; i >= 1; --i) {
        __CTOR_LIST__[i]();
    }

    STARTUPINFOA startup;
    startup.cb = sizeof(startup);
    GetStartupInfoA(&startup);
    const int show = (startup.dwFlags & STARTF_USESHOWWINDOW) ? startup.wShowWindow : SW_SHOWDEFAULT;

    const int exitCode = WinMain(GetModuleHandleA(nullptr), nullptr, SkipProgramName(GetCommandLineA()), show);
    ExitProcess(static_cast<UINT>(exitCode));
}
