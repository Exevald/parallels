add_library(timer_lib STATIC
        Timer.cpp
)

target_include_directories(timer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})