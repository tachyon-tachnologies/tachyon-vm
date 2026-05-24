cmake_minimum_required(VERSION 3.20)

if(MSVC)
    message(FATAL_ERROR "Please use a compiler such as G++ or Clang++")
endif()

project(
    tachyon-vm
    VERSION 0.01
    DESCRIPTION "Tachyon's VM"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TACHYON_SOURCE "${CMAKE_SOURCE_DIR}/Source")

file(GLOB_RECURSE CPP_SRC FOLLOW_SYMLINKS "${TACHYON_SOURCE}/*.cpp")

add_executable(${PROJECT_NAME} ${CPP_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE "${TACHYON_SOURCE}/Include")

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wextra
    -Wpointer-arith
    -Wcast-align
    -Wwrite-strings
    -Wredundant-decls
    -Wformat
    -Wformat-security
    -Wno-missing-field-initializers
    -fno-rtti
    -fno-exceptions
    -march=native
)

# SDl3
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

# libzip
find_package(libzip CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libzip::zip)
