cmake_minimum_required(VERSION 3.16)

project(MiniGit
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
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

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include current directory for headers
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link dependencies from vcpkg
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)