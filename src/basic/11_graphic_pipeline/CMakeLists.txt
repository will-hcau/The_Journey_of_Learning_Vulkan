find_package(Vulkan REQUIRED)

find_program(SLANGC_EXECUTABLE NAMES slangc HINTS "$ENV{VULKAN_SDK}/bin" "${Vulkan_ROOT}/bin")
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc was not found. Install the Vulkan SDK or set VULKAN_SDK.")
endif()

set(EXAMPLE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic/11_graphic_pipeline")
set(SHADER_BINARY "${EXAMPLE_OUTPUT_DIRECTORY}/slang.spv")
add_custom_command(
    OUTPUT "${SHADER_BINARY}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${EXAMPLE_OUTPUT_DIRECTORY}"
    COMMAND "${SLANGC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.slang"
            -target spirv -profile spirv_1_4 -emit-spirv-directly
            -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o "${SHADER_BINARY}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.slang"
    VERBATIM)

add_executable(graphic_pipeline
    main.cpp src/graphicsPipeline.cpp src/device.cpp src/swapchain.cpp
    src/vulkan_instance.cpp src/window.cpp "${SHADER_BINARY}")
set_target_properties(graphic_pipeline PROPERTIES OUTPUT_NAME "11_graphic_pipeline" RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIRECTORY}")
target_compile_features(graphic_pipeline PRIVATE cxx_std_20)
target_include_directories(graphic_pipeline PRIVATE include "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(graphic_pipeline PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
if(VULKAN_ENABLE_VALIDATION)
    target_compile_definitions(graphic_pipeline PRIVATE VULKAN_ENABLE_VALIDATION)
endif()
target_link_libraries(graphic_pipeline PRIVATE glfw Vulkan::Vulkan)
if(MSVC)
    target_compile_options(graphic_pipeline PRIVATE /W4 /permissive-)
else()
    target_compile_options(graphic_pipeline PRIVATE -Wall -Wextra -Wpedantic)
endif()
