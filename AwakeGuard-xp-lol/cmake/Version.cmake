# Parse AWAKEGUARD_VERSION (e.g. 0.1.2 from tag v0.1.2) for resources and About UI.

if(NOT DEFINED AWAKEGUARD_VERSION OR AWAKEGUARD_VERSION STREQUAL "")
    set(AWAKEGUARD_VERSION "0.1.1" CACHE STRING "Semantic version (MAJOR.MINOR.PATCH)")
endif()

set(_awakeguard_version "${AWAKEGUARD_VERSION}")
if(_awakeguard_version MATCHES "^v(.+)$")
    set(_awakeguard_version "${CMAKE_MATCH_1}")
endif()

if(NOT _awakeguard_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
        "AWAKEGUARD_VERSION must be MAJOR.MINOR.PATCH (got '${AWAKEGUARD_VERSION}')")
endif()

string(REPLACE "." ";" _version_parts "${_awakeguard_version}")
list(GET _version_parts 0 AWAKEGUARD_VERSION_MAJOR)
list(GET _version_parts 1 AWAKEGUARD_VERSION_MINOR)
list(GET _version_parts 2 AWAKEGUARD_VERSION_PATCH)
set(AWAKEGUARD_VERSION_REVISION "0")

# Match Directory.Build.props: four-part file version for VERSIONINFO.
set(AWAKEGUARD_FILE_VERSION_STR
    "${AWAKEGUARD_VERSION_MAJOR}.${AWAKEGUARD_VERSION_MINOR}.${AWAKEGUARD_VERSION_PATCH}.${AWAKEGUARD_VERSION_REVISION}-XP-LOL")
set(AWAKEGUARD_PRODUCT_VERSION_STR
    "${AWAKEGUARD_VERSION_MAJOR}.${AWAKEGUARD_VERSION_MINOR}.${AWAKEGUARD_VERSION_PATCH}-XP-LOL")
set(AWAKEGUARD_MANIFEST_VERSION
    "${AWAKEGUARD_VERSION_MAJOR}.${AWAKEGUARD_VERSION_MINOR}.${AWAKEGUARD_VERSION_PATCH}.${AWAKEGUARD_VERSION_REVISION}")

set(AWAKEGUARD_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${AWAKEGUARD_GENERATED_DIR}")

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/awakeguard_version.h.in"
    "${AWAKEGUARD_GENERATED_DIR}/awakeguard_version.h"
    @ONLY)

configure_file(
    "${CMAKE_SOURCE_DIR}/app.manifest.in"
    "${AWAKEGUARD_GENERATED_DIR}/app.manifest"
    @ONLY)

message(STATUS "AwakeGuard XP LOL version: ${AWAKEGUARD_PRODUCT_VERSION_STR}")
