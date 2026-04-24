cmake_minimum_required(VERSION 3.27)

add_library(bank_lib STATIC
        Bank.cpp
        Simulation.cpp
        main.cpp
)

add_executable(bank
        Bank.cpp
        Simulation.cpp
        main.cpp
)

target_include_directories(bank_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})