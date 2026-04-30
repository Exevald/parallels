include(GoogleTest)

add_executable(atomic_max_test
		AtomicMaxTests.cpp
)

target_link_libraries(atomic_max_test PRIVATE GTest::gtest_main atomic_max_lib)
gtest_discover_tests(atomic_max_test)
