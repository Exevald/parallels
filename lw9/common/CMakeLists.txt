find_package(OpenCL REQUIRED)

add_library(gpu-runner
        GpuRunner.cpp
)

target_include_directories(gpu-runner PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/lib/opencl
)

target_compile_definitions(gpu-runner PUBLIC
        CL_HPP_ENABLE_EXCEPTIONS
        CL_HPP_TARGET_OPENCL_VERSION=120
        CL_HPP_MINIMUM_OPENCL_VERSION=120
)

target_link_libraries(gpu-runner PUBLIC OpenCL::OpenCL)
