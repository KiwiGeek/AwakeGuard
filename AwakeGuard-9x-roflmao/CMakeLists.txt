cmake_minimum_required(VERSION 3.20)

project(AwakeGuard-9x-ROFLMAO LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR
        "Use the MinGW 9x toolchain file, e.g.\n"
        "  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686-9x.cmake")
endif()

include(cmake/Version.cmake)

foreach(_icon IN ITEMS AwakeGuard.ico tray-active.ico tray-inactive.ico)
    if(NOT EXISTS "${AWAKEGUARD_ASSETS_DIR}/${_icon}")
        message(FATAL_ERROR "Missing icon ${AWAKEGUARD_ASSETS_DIR}/${_icon} (run scripts/generate-icons.py)")
    endif()
endforeach()

# No C/C++ runtime is linked (-nostdlib below). MSYS2's prebuilt CRT, libgcc
# and libstdc++ are compiled for Pentium 4 (LOCK CMPXCHG, CMOV, SSE), which
# a 386/486 can't run and Windows 95 can't run on any CPU (it doesn't save SSE
# state). Every instruction in the exe comes from these sources, built with
# -march=i386; scripts/check-9x-exe.py enforces that in CI.
add_executable(AwakeGuard-9x-ROFLMAO WIN32
    src/rt_entry.cpp
    src/rt_mem.c
    src/str.cpp
    src/main.cpp
    src/app.cpp
    src/session.cpp
    src/os_compat.cpp
    src/paths.cpp
    src/path_util.cpp
    src/settings_store.cpp
    src/startup.cpp
    src/cleanup.cpp
    src/tray.cpp
    src/settings_wnd.cpp
    src/dialogs.cpp
    src/util.cpp
    src/ui.cpp
    src/resource.rc
    "${AWAKEGUARD_GENERATED_DIR}/app.manifest"
)

target_include_directories(AwakeGuard-9x-ROFLMAO PRIVATE
    src
    "${AWAKEGUARD_GENERATED_DIR}"
)

if(CMAKE_RC_COMPILER)
    set_source_files_properties(src/resource.rc PROPERTIES
        COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/src -I${AWAKEGUARD_GENERATED_DIR}")
endif()

target_compile_definitions(AwakeGuard-9x-ROFLMAO PRIVATE
    AWAKEGUARD_9X_ROFLMAO=1
    WINVER=0x0400
    _WIN32_WINDOWS=0x0400
    _WIN32_WINNT=0x0400
)

# Import libraries only; every DLL here ships with Windows 95 RTM.
target_link_libraries(AwakeGuard-9x-ROFLMAO PRIVATE
    kernel32
    user32
    gdi32
    shell32
    advapi32
    comctl32
)

target_compile_options(AwakeGuard-9x-ROFLMAO PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector -fcf-protection=none>
    # Keep the assembler from using P6-only multi-byte NOPs (0F 1F) for padding.
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wa,-march=i386>
)

# Stop GCC from compiling these loops into calls to memcpy/memset themselves.
set_source_files_properties(src/rt_mem.c PROPERTIES
    COMPILE_OPTIONS "-fno-builtin;-fno-tree-loop-distribute-patterns")

target_link_options(AwakeGuard-9x-ROFLMAO PRIVATE
    -nostdlib
    -s
    "-Wl,--subsystem,windows:4.00"
    "-Wl,-e,_WinMainCRTStartup"
)

set_target_properties(AwakeGuard-9x-ROFLMAO PROPERTIES
    OUTPUT_NAME "AwakeGuard-9x-ROFLMAO"
    WIN32_EXECUTABLE TRUE
)
