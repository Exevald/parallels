add_library(lw7_thread_pool_lib STATIC
		LockBasedThreadPool.cpp
		LockFreeThreadPool.cpp
)

target_include_directories(lw7_thread_pool_lib
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(lw7_thread_pool_lib SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_compile_definitions(lw7_thread_pool_lib PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)
