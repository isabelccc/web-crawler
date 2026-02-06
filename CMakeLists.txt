cmake_minimum_required(VERSION 3.20)
project(web-crawler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(USE_SANITIZERS "Use sanitizers" OFF)

if(USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
endif()

# Find packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Crow (HTTP server)
include(FetchContent)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.0+5
)
FetchContent_MakeAvailable(crow)

# Redis (hiredis)
pkg_check_modules(HIREDIS REQUIRED hiredis)

# JSON (nlohmann/json)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# HTML parsing (Gumbo)
pkg_check_modules(GUMBO REQUIRED gumbo)

# xxhash for content hashing
find_library(XXHASH_LIB xxhash)
if(NOT XXHASH_LIB)
    message(WARNING "xxhash not found, using fallback")
endif()

# OpenSSL for SHA256
find_package(OpenSSL REQUIRED)

# libcurl for HTTP client
find_package(CURL REQUIRED)

# yaml-cpp for config parsing
find_package(yaml-cpp REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${HIREDIS_INCLUDE_DIRS}
    ${GUMBO_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/scheduler/scheduler.cpp
    src/fetcher/fetcher.cpp
    src/parser/parser.cpp
    src/dedup/dedup.cpp
    src/indexer/indexer.cpp
    src/storage/storage.cpp
    src/api/api_server.cpp
    src/observability/metrics.cpp
    src/observability/logger.cpp
    src/utils/url_utils.cpp
    src/utils/hash_utils.cpp
    src/utils/config.cpp
)

# Headers
set(HEADERS
    src/scheduler/scheduler.h
    src/fetcher/fetcher.h
    src/parser/parser.h
    src/dedup/dedup.h
    src/indexer/indexer.h
    src/storage/storage.h
    src/api/api_server.h
    src/observability/metrics.h
    src/observability/logger.h
    src/utils/url_utils.h
    src/utils/hash_utils.h
    src/utils/config.h
)

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Threads::Threads
    ${HIREDIS_LIBRARIES}
    ${GUMBO_LIBRARIES}
    ${XXHASH_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    yaml-cpp
    crow::crow
)

target_compile_options(${PROJECT_NAME} PRIVATE ${HIREDIS_CFLAGS_OTHER} ${GUMBO_CFLAGS_OTHER})

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY configs/ DESTINATION etc/web-crawler)
