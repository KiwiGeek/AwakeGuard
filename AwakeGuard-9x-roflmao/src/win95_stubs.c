/*
 * Stubs for KERNEL32 exports that libgcc/libwinpthread use but Win95 lacks.
 * libwinpthread imports __imp__*@N; provide those pointers (not KERNEL32 thunks).
 */
#include <windows.h>

BOOL WINAPI IsProcessorFeaturePresent(DWORD feature) {
    (void)feature;
    return FALSE;
}

static BOOL WINAPI ag_GetProcessAffinityMask(HANDLE process, PDWORD processMask, PDWORD systemMask) {
    (void)process;
    if (processMask) {
        *processMask = 1;
    }
    if (systemMask) {
        *systemMask = 1;
    }
    return TRUE;
}

static BOOL WINAPI ag_SetProcessAffinityMask(HANDLE process, DWORD mask) {
    (void)process;
    (void)mask;
    return TRUE;
}

static BOOL WINAPI ag_TryEnterCriticalSection(LPCRITICAL_SECTION section) {
    EnterCriticalSection(section);
    return TRUE;
}

BOOL (WINAPI *imp_GetProcessAffinityMask)(HANDLE, PDWORD, PDWORD)
    asm("__imp__GetProcessAffinityMask@12") = ag_GetProcessAffinityMask;

BOOL (WINAPI *imp_SetProcessAffinityMask)(HANDLE, DWORD)
    asm("__imp__SetProcessAffinityMask@8") = ag_SetProcessAffinityMask;

BOOL (WINAPI *imp_TryEnterCriticalSection)(LPCRITICAL_SECTION)
    asm("__imp__TryEnterCriticalSection@4") = ag_TryEnterCriticalSection;
