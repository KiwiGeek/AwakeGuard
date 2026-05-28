# Cross-compile a 32-bit Windows 9x-oriented binary from Linux CI or MSYS2.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

find_program(_MINGW_CXX NAMES i686-w64-mingw32-g++ i686-w64-mingw32-g++-win32)
find_program(_MINGW_CC NAMES i686-w64-mingw32-gcc i686-w64-mingw32-gcc-win32)
find_program(_MINGW_RC NAMES i686-w64-mingw32-windres windres)

if(NOT _MINGW_CXX OR NOT _MINGW_CC)
    message(FATAL_ERROR "MinGW i686 cross-compiler not found. Install g++-mingw-w64-i686-win32 (Ubuntu) or mingw-w64-i686-gcc (MSYS2).")
endif()

set(CMAKE_C_COMPILER "${_MINGW_CC}")
set(CMAKE_CXX_COMPILER "${_MINGW_CXX}")
if(_MINGW_RC)
    set(CMAKE_RC_COMPILER "${_MINGW_RC}")
else()
    set(CMAKE_RC_COMPILER windres)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Link MSVCRT20.DLL (ships in Win95 System) instead of MSVCRT.DLL (VC6 redist).
set(_AWAKEGUARD_9X_CRT "-mcrtdll=msvcrt20")

# i486 avoids libgcc paths that import IsProcessorFeaturePresent (Win98+).
set(_AWAKEGUARD_9X_ARCH "-march=i486 -mtune=i486")

set(CMAKE_C_FLAGS_INIT "-Os ${_AWAKEGUARD_9X_CRT} ${_AWAKEGUARD_9X_ARCH} -DWINVER=0x0400 -D_WIN32_WINDOWS=0x0400 -D_WIN32_WINNT=0x0400")
set(CMAKE_CXX_FLAGS_INIT "-Os ${_AWAKEGUARD_9X_CRT} ${_AWAKEGUARD_9X_ARCH} -DWINVER=0x0400 -D_WIN32_WINDOWS=0x0400 -D_WIN32_WINNT=0x0400")
