cmake_minimum_required(VERSION 3.21)
project(RayTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(RayTracer
    src/main.cpp
)

# Include directories
target_include_directories(RayTracer PRIVATE
    include/
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Find SDL3
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0 
)
FetchContent_MakeAvailable(SDL3)
target_link_libraries(RayTracer PRIVATE SDL3::SDL3)

# OpenCL Headers
FetchContent_Declare(
    OpenCLHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-Headers.git
    GIT_TAG main
)
FetchContent_MakeAvailable(OpenCLHeaders)

# Only download what I need (opencl.hpp), and not the full repo
file(DOWNLOAD
    https://raw.githubusercontent.com/KhronosGroup/OpenCL-CLHPP/main/include/CL/opencl.hpp
    ${CMAKE_BINARY_DIR}/include/CL/opencl.hpp
)

target_include_directories(RayTracer PRIVATE
    ${openclheaders_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

FetchContent_Declare(
    OpenCLICDLoader
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-ICD-Loader.git
    GIT_TAG main
)
FetchContent_MakeAvailable(OpenCLICDLoader)

target_link_libraries(RayTracer PRIVATE OpenCL::OpenCL)

# Handle OpenCL kernels - embed them as strings
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/embedded_kernels.h
    COMMAND ${CMAKE_COMMAND} -DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/kernels -DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/embedded_kernels.h -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedKernels.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kernels/common.cl
            ${CMAKE_CURRENT_SOURCE_DIR}/kernels/ray.cl
            ${CMAKE_CURRENT_SOURCE_DIR}/kernels/render.cl
    COMMENT "Embedding OpenCL kernels"
)

add_custom_command(TARGET RayTracer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:RayTracer>
        $<TARGET_FILE_DIR:RayTracer>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Copying runtime DLLs to build directory"
)
target_sources(RayTracer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/embedded_kernels.h)
target_include_directories(RayTracer PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(embed_kernels DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/embedded_kernels.h)
add_dependencies(RayTracer embed_kernels)
