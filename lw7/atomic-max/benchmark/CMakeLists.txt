add_executable(atomic_max_benchmark
		AtomicMaxBenchmark.cpp
)

target_link_libraries(atomic_max_benchmark PRIVATE atomic_max_lib timer_lib)
