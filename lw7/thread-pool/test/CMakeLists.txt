include(GoogleTest)

add_executable(lockfree_thread_pool_test
		ThreadPoolTests.cpp
)

target_link_libraries(lockfree_thread_pool_test PRIVATE GTest::gtest_main lw7_thread_pool_lib)
gtest_discover_tests(lockfree_thread_pool_test)
