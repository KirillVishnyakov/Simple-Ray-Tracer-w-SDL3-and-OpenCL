# cmake/EmbedKernels.cmake
# Script to convert .cl files to C++ header

# Read all kernel files and create a C++ header
file(WRITE ${OUTPUT_FILE} "#pragma once\n#include <string>\n\nnamespace Kernels {\n")

# Get all .cl files from input directory
file(GLOB KERNEL_FILES "${INPUT_DIR}/*.cl")

foreach(kernel_file ${KERNEL_FILES})
    # Read kernel file content
    file(READ ${kernel_file} kernel_content)
    
    # Get filename and create variable name
    get_filename_component(kernel_name ${kernel_file} NAME)
    string(REPLACE "." "_" var_name ${kernel_name})
    
    # Escape special characters for C++ string
    string(REPLACE "\\" "\\\\" kernel_content "${kernel_content}")
    string(REPLACE "\"" "\\\"" kernel_content "${kernel_content}")
    string(REPLACE "\n" "\\n\"\n\"" kernel_content "${kernel_content}")
    
    # Write to output file
    file(APPEND ${OUTPUT_FILE} "const std::string ${var_name} = \n\"${kernel_content}\";\n\n")
endforeach()

file(APPEND ${OUTPUT_FILE} "} // namespace Kernels\n")