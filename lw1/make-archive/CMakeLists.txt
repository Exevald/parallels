add_executable(
        make-archive
        ArgsParser.cpp
        TempFileManager.cpp
        Compressor.cpp
        main.cpp
        Archiver.cpp
)

target_link_libraries(make-archive PRIVATE filedesc_lib timer_lib)