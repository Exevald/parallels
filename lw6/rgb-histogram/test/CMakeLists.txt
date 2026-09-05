include(GoogleTest)

add_executable(rgb_histogram_test
        HistogramTests.cpp
)

target_link_libraries(rgb_histogram_test PRIVATE GTest::gtest_main rgb_histogram_lib)
gtest_discover_tests(rgb_histogram_test)
