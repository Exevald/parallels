add_executable(
        extract-files
        ArgsParser.cpp
        main.cpp
        Extractor.cpp
        Decompressor.cpp
        FileScanner.cpp
)

target_link_libraries(extract-files PRIVATE filedesc_lib timer_lib)