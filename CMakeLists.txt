cmake_minimum_required(VERSION 3.20)

project(
    tachyon-vm
    VERSION 0.01
    DESCRIPTION "Tachyon's VM"
    LANGUAGES CXX
)

if(MSVC)
    message(FATAL_ERROR "Please use a compiler such as G++ or Clang++")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_UNITY_BUILD ON)

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TACHYON_SOURCE "${CMAKE_SOURCE_DIR}/Source")

file(GLOB_RECURSE CPP_SRC FOLLOW_SYMLINKS "${TACHYON_SOURCE}/*.cpp")

add_executable(${PROJECT_NAME} ${CPP_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE "${TACHYON_SOURCE}/Include")

target_compile_options(${PROJECT_NAME} PRIVATE
    -Werror
    -Wextra
    -fno-rtti
    -fno-exceptions
    -march=native
    -D_WIN32_WINNT=0x0A00
)

target_link_options(${PROJECT_NAME} PRIVATE
    -ldl
)

# SDl3
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

# libzip
find_package(libzip CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libzip::zip)

# simdjson
find_package(simdjson CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE simdjson::simdjson)
