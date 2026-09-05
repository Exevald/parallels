include(GoogleTest)

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost REQUIRED)

add_library(radio_core_lib STATIC
        src/Protocol.cpp
        src/TcpChannel.cpp
)

target_include_directories(radio_core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(radio_core_lib SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_compile_definitions(radio_core_lib PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)

add_executable(radio
        src/Main.cpp
        src/Cli.cpp
        src/Station.cpp
        src/Receiver.cpp
        src/AudioRecorder.cpp
        src/AudioPlaybackStream.cpp
        src/AudioVisualizer.cpp
)

target_link_libraries(
        radio
        PRIVATE
        radio_core_lib
        timer_lib
        SFML::Audio
)

add_executable(radio_protocol_test
        test/ProtocolTests.cpp
)

target_link_libraries(radio_protocol_test PRIVATE GTest::gtest_main radio_core_lib)
gtest_discover_tests(radio_protocol_test)
