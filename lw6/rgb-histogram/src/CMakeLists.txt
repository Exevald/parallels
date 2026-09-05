add_library(rgb_histogram_lib STATIC
        Histogram.cpp
        ImageLoader.cpp
)

target_include_directories(rgb_histogram_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rgb_histogram_lib PUBLIC stb_image_lib)

add_executable(rgb-histogram
        main.cpp
)

target_link_libraries(rgb-histogram PRIVATE  rgb_histogram_lib timer_lib)
