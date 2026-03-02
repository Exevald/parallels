add_executable(
        gauss
        ImageLoader.cpp
        GaussianBlur.cpp
        ArgsParser.cpp
        main.cpp
)

target_link_libraries(gauss PRIVATE stb_image_lib deferrer_lib)