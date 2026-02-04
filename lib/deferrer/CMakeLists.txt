add_library(deferrer_lib STATIC
        deferrer.cpp
)

target_include_directories(deferrer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})