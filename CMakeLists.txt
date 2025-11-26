cmake_minimum_required(VERSION 3.16)

project(MiniGit
    VERSION 1.0.0
    DESCRIPTION "A lightweight git-like system"
    LANGUAGES CXX
)

# ---------------------------------------------------------
# C++ standard
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------
# Source files
# ---------------------------------------------------------
set(SOURCES
    main.cpp
    Commit.cpp
    Log.cpp
    Repository.cpp
)

set(HEADERS
    Commit.h
    Log.h
    Repository.h
    MiniGit.h
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---------------------------------------------------------
# External dependency: nlohmann/json (header-only)
# ---------------------------------------------------------
# If installed from system package manager:
#   Ubuntu: sudo apt install nlohmann-json3-dev
#   MacOS Homebrew: brew install nlohmann-json
#
# If using a package manager like vcpkg:
#   vcpkg install nlohmann-json
#
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# ---------------------------------------------------------
# External dependency: OpenSSL (for SHA)
# ---------------------------------------------------------
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto)

# Optional: For debugging messages
message(STATUS "Using OpenSSL version: ${OPENSSL_VERSION}")