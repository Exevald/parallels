cmake_minimum_required(VERSION 3.27)

if (CMAKE_HOST_APPLE)
    # Homebrew libraries use Apple's libc++, so the whole project uses one
    # compatible compiler toolchain on macOS.
    set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(parallels)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(SFML REQUIRED COMPONENTS Audio Graphics Window System)

add_subdirectory(lib)
add_subdirectory(lw1)
add_subdirectory(lw2)
add_subdirectory(lw3)
add_subdirectory(lw4)
add_subdirectory(lw5)
add_subdirectory(lw6)
add_subdirectory(lw7)
add_subdirectory(lw8)
add_subdirectory(lw9)
