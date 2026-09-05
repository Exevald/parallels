find_package(TBB REQUIRED)

find_path(ONEDPL_INCLUDE_DIR
        NAMES oneapi/dpl/execution
        HINTS
        /opt/homebrew/include
        /usr/local/include
)

if (NOT ONEDPL_INCLUDE_DIR)
    message(FATAL_ERROR "oneDPL was not found. Install oneDPL and configure CMake again.")
endif()

add_executable(radix-sort
        GpuRadixSort.cpp
        main.cpp
)

target_include_directories(radix-sort PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ONEDPL_INCLUDE_DIR}
)

target_link_libraries(radix-sort PRIVATE
        gpu-runner
        TBB::tbb
)
