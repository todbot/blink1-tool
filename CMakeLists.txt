cmake_minimum_required(VERSION 3.15)

# Suppress false errors from the bundled hidapi submodule (its cmake_minimum_required
# uses a min of 3.1.3, which newer CMake rejects unless this override is set).
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)

# macOS universal binary — must be set before project() so the compiler
# detection tests are run for all target architectures.
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS build architectures")
endif()
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum macOS deployment target")
endif()

project(blink1-tool LANGUAGES C)

# --- Version ---
# Read from most recent git tag; fall back to date stamp if no tags exist
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE BLINK1_GIT_TAG
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT BLINK1_GIT_TAG MATCHES "^v")
    string(TIMESTAMP BLINK1_GIT_TAG "v%Y%m%d")
endif()

if(APPLE)
    set(BLINK1_VERSION "${BLINK1_GIT_TAG}-macosx")
elseif(WIN32)
    set(BLINK1_VERSION "${BLINK1_GIT_TAG}-windows")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(BLINK1_VERSION "${BLINK1_GIT_TAG}-linux-${CMAKE_SYSTEM_PROCESSOR}")
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(BLINK1_VERSION "${BLINK1_GIT_TAG}-freebsd-${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(BLINK1_VERSION "${BLINK1_GIT_TAG}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()
message(STATUS "blink1 version: ${BLINK1_VERSION}")

include(FetchContent)

# --- Build hidapi as a static library ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# --- Linux: HIDAPI backend selection ---
# hidraw is the Makefile default and requires only libudev (no libusb).
# Override with -DBLINK1_HIDAPI_LIBUSB=ON to use the libusb backend instead.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(BLINK1_HIDAPI_LIBUSB "Use libusb HIDAPI backend instead of hidraw" OFF)
    if(BLINK1_HIDAPI_LIBUSB)
        set(HIDAPI_WITH_HIDRAW OFF CACHE BOOL "" FORCE)
        set(HIDAPI_WITH_LIBUSB ON  CACHE BOOL "" FORCE)
    else()
        set(HIDAPI_WITH_HIDRAW ON  CACHE BOOL "" FORCE)
        set(HIDAPI_WITH_LIBUSB OFF CACHE BOOL "" FORCE)
    endif()
endif()

# --- HIDAPI from bundled git submodule (hidapi/) ---
add_subdirectory(hidapi EXCLUDE_FROM_ALL)

# --- blink1-lib: core library (static) ---
add_library(blink1-lib STATIC blink1-lib.c)

target_include_directories(blink1-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(blink1-lib
    PUBLIC  USE_HIDAPI          # blink1-lib.h checks this; propagate to all consumers
    PRIVATE BLINK1_VERSION="${BLINK1_VERSION}"
            $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

target_compile_options(blink1-lib PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/W3,-Wall>)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_options(blink1-lib PRIVATE
        -fPIC
        -Wno-format
        -Wno-pointer-to-int-cast
    )
endif()

# hidapi::hidapi is a platform alias: darwin on macOS, winapi on Windows,
# hidraw (or libusb) on Linux. The macOS IOKit/CoreFoundation/AppKit frameworks
# are PRIVATE deps of hidapi_darwin and propagate automatically to the final link.
target_link_libraries(blink1-lib PUBLIC hidapi::hidapi)

if(WIN32)
    # hidapi_winapi does not pull setupapi through its CMake target
    target_link_libraries(blink1-lib PUBLIC setupapi)
endif()

# --- blink1-tool: CLI executable ---
add_executable(blink1-tool blink1-tool.c)

target_compile_definitions(blink1-tool PRIVATE
    BLINK1_VERSION="${BLINK1_VERSION}"
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

target_compile_options(blink1-tool PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/W3,-Wall>)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_options(blink1-tool PRIVATE -Wno-format -Wno-pointer-to-int-cast)
endif()

target_link_libraries(blink1-tool PRIVATE blink1-lib)

# Windows (MSVC): getopt is not in the MSVC runtime; fetch a port
if(MSVC)
    FetchContent_Declare(
        getopt_win
        GIT_REPOSITORY https://github.com/ludvikjerabek/getopt-win.git
        GIT_TAG v2.42.0
    )
    FetchContent_MakeAvailable(getopt_win)
    target_link_libraries(blink1-tool PRIVATE getopt::getopt_static)
    target_compile_definitions(blink1-tool PRIVATE STATIC_GETOPT)
endif()

# Windows (MinGW): static C runtime so the .exe runs without MinGW DLLs
if(WIN32 AND NOT MSVC)
    target_link_options(blink1-tool PRIVATE
        -static-libgcc
        -Wl,--enable-auto-import
    )
    target_link_libraries(blink1-tool PRIVATE ws2_32)
endif()

# Linux: opt-in fully-static link.
# glibc static linking is broken on Ubuntu 13+; use a musl toolchain (e.g. Alpine).
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(BLINK1_STATIC_LINK "Link blink1-tool fully static (requires musl, not glibc)" OFF)
    if(BLINK1_STATIC_LINK)
        target_link_options(blink1-tool PRIVATE -static)
    endif()
endif()

# --- blink1-tiny-server: HTTP REST server (Unix only; MSVC lacks getopt/sys/time/signal) ---
if(NOT MSVC)

# pack: host tool that reads files and emits them as C byte arrays.
# It supports -s STRIP_PREFIX to remove the source-tree path from embedded names.
add_executable(pack server/mongoose/pack.c)

# Gather HTML files that get packed into the binary.
file(GLOB_RECURSE BLINK1_SERVER_HTML_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/server/html/*"
)

set(BLINK1_SERVER_HTML_C "${CMAKE_CURRENT_BINARY_DIR}/blink1-tiny-server-html.c")

# Write a small cmake script into the build tree so add_custom_command can
# redirect pack's stdout to the output file (pipes aren't portable in COMMAND).
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pack_html.cmake" [[
execute_process(
    COMMAND "${PACK_EXE}" -s "${STRIP_PREFIX}" ${HTML_FILES}
    OUTPUT_FILE "${OUTPUT_FILE}"
    RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "pack failed with exit code ${_result}")
endif()
]])

add_custom_command(
    OUTPUT "${BLINK1_SERVER_HTML_C}"
    COMMAND ${CMAKE_COMMAND}
        -D "PACK_EXE=$<TARGET_FILE:pack>"
        -D "STRIP_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/server/html/"
        -D "OUTPUT_FILE=${BLINK1_SERVER_HTML_C}"
        -D "HTML_FILES=${BLINK1_SERVER_HTML_FILES}"
        -P "${CMAKE_CURRENT_BINARY_DIR}/pack_html.cmake"
    DEPENDS pack ${BLINK1_SERVER_HTML_FILES}
    COMMENT "Packing server/html into C"
    VERBATIM
)

add_executable(blink1-tiny-server
    server/blink1-tiny-server.c
    server/mongoose/mongoose.c
    server/parson/parson.c
    "${BLINK1_SERVER_HTML_C}"
)

target_include_directories(blink1-tiny-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/server/mongoose"
    "${CMAKE_CURRENT_SOURCE_DIR}/server/parson"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_definitions(blink1-tiny-server PRIVATE
    MG_ENABLE_PACKED_FS=1
    BLINK1_VERSION="${BLINK1_VERSION}"
)

target_compile_options(blink1-tiny-server PRIVATE
    -Wall -Wno-format -Wno-pointer-to-int-cast
)

target_link_libraries(blink1-tiny-server PRIVATE blink1-lib)

if(WIN32 AND NOT MSVC)
    target_link_options(blink1-tiny-server PRIVATE -static-libgcc -Wl,--enable-auto-import)
    target_link_libraries(blink1-tiny-server PRIVATE ws2_32)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND BLINK1_STATIC_LINK)
    target_link_options(blink1-tiny-server PRIVATE -static)
endif()

endif() # NOT MSVC
