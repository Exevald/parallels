add_executable(rgb_histogram_benchmark
        HistogramBenchmarks.cpp
)

target_link_libraries(rgb_histogram_benchmark PRIVATE rgb_histogram_lib timer_lib)
