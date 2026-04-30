include(GoogleTest)

if (POLICY CMP0167)
	cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(benchmark)
