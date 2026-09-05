add_executable(gaussian-blur
        GaussianBlurWindow.cpp
        GpuGaussianBlur.cpp
        main.cpp
)

target_include_directories(gaussian-blur PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gaussian-blur PRIVATE
        gpu-runner
        SFML::Graphics
        SFML::Window
        SFML::System
)
