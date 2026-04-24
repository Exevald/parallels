add_executable(
        sound-player
        common/StringUtils.cpp
        io/AudioPlayer.cpp
        io/WavWriter.cpp
        parser/MelodyParser.cpp
        render/SoundRenderer.cpp
        ui/WaveformVisualizer.cpp
        main.cpp
)

target_include_directories(
        sound-player
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
