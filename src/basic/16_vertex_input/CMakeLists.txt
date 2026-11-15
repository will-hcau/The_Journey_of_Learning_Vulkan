find_package(Vulkan REQUIRED)

find_program(SLANGC_EXECUTABLE NAMES slangc HINTS "$ENV{VULKAN_SDK}/bin" "${Vulkan_ROOT}/bin")
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc was not found. Install the Vulkan SDK or set VULKAN_SDK.")
endif()

set(EXAMPLE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic/16_vertex_input")
set(SHADER_BINARY "${EXAMPLE_OUTPUT_DIRECTORY}/slang.spv")
add_custom_command(
    OUTPUT "${SHADER_BINARY}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${EXAMPLE_OUTPUT_DIRECTORY}"
    COMMAND "${SLANGC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.slang"
            -target spirv -profile spirv_1_4 -emit-spirv-directly
            -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o "${SHADER_BINARY}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.slang"
    VERBATIM)

add_executable(vertex_input
    main.cpp src/commandBuffer.cpp src/graphicsPipeline.cpp src/device.cpp src/swapchain.cpp
    src/vulkan_instance.cpp src/window.cpp src/vertex.cpp "${SHADER_BINARY}")
set_target_properties(vertex_input PROPERTIES OUTPUT_NAME "16_vertex_input" RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIRECTORY}")
target_compile_features(vertex_input PRIVATE cxx_std_20)
target_include_directories(vertex_input PRIVATE include "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(vertex_input PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
if(VULKAN_ENABLE_VALIDATION)
    target_compile_definitions(vertex_input PRIVATE VULKAN_ENABLE_VALIDATION)
endif()
target_link_libraries(vertex_input PRIVATE glfw Vulkan::Vulkan)
if(MSVC)
    target_compile_options(vertex_input PRIVATE /W4 /permissive-)
else()
    target_compile_options(vertex_input PRIVATE -Wall -Wextra -Wpedantic)
endif()
