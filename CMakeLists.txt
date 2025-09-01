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
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
target_link_libraries(RayTracer PRIVATE SDL3::SDL3)

# Find OpenCL
find_package(OpenCL REQUIRED)
target_include_directories(RayTracer PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(RayTracer PRIVATE ${OpenCL_LIBRARIES})

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
