cmake_minimum_required(VERSION 3.27)
project(parallels)

set(CMAKE_CXX_STANDARD 20)
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
