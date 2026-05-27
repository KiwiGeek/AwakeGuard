cmake_minimum_required(VERSION 3.20)
project(AwakeGuard-win32 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AwakeGuard WIN32
    src/main.cpp
    src/app.cpp
    src/session.cpp
    src/paths.cpp
    src/settings_store.cpp
    src/startup.cpp
    src/cleanup.cpp
    src/tray.cpp
    src/settings_wnd.cpp
    src/dialogs.cpp
    src/util.cpp
    src/resource.rc
)

target_include_directories(AwakeGuard PRIVATE src)

target_link_libraries(AwakeGuard PRIVATE
    user32 gdi32 shell32 advapi32 bcrypt comctl32 ole32 shlwapi
)

if(MSVC)
    target_compile_options(AwakeGuard PRIVATE /W4 /utf-8)
    set_target_properties(AwakeGuard PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/app.manifest"
    )
endif()
