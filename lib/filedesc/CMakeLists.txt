add_library(filedesc_lib STATIC
        FileDesc.cpp
)

target_include_directories(filedesc_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})