cmake_minimum_required(VERSION 3.20)

project(AwakeGuard-XP-LOL LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR
        "Use the MinGW XP toolchain file, e.g.\n"
        "  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686-xp.cmake")
endif()

add_executable(AwakeGuard-XP-LOL WIN32
    src/main.cpp
    src/app.cpp
    src/session.cpp
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
)

target_include_directories(AwakeGuard-XP-LOL PRIVATE src)

target_compile_definitions(AwakeGuard-XP-LOL PRIVATE
    AWAKEGUARD_XP_LOL=1
    WINVER=0x0501
    _WIN32_WINNT=0x0501
    UNICODE
    _UNICODE
    AWAKEGUARD_VERSION_MAJOR=0
    AWAKEGUARD_VERSION_MINOR=1
    AWAKEGUARD_VERSION_PATCH=1
    AWAKEGUARD_VERSION_REVISION=0
    AWAKEGUARD_FILE_VERSION_STR=\"0.1.1.0-XP-LOL\"
)

target_link_libraries(AwakeGuard-XP-LOL PRIVATE
    user32
    gdi32
    shell32
    advapi32
    comctl32
    ole32
    shlwapi
)

target_link_options(AwakeGuard-XP-LOL PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
    -municode
    "-Wl,--subsystem,windows:5.01"
)

set_target_properties(AwakeGuard-XP-LOL PROPERTIES
    OUTPUT_NAME "AwakeGuard-XP-LOL"
    WIN32_EXECUTABLE TRUE
)

if(MINGW AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app.manifest")
    target_sources(AwakeGuard-XP-LOL PRIVATE app.manifest)
endif()
