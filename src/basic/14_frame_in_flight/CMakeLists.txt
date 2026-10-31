find_package(Vulkan REQUIRED)

find_program(SLANGC_EXECUTABLE NAMES slangc HINTS "$ENV{VULKAN_SDK}/bin" "${Vulkan_ROOT}/bin")
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc was not found. Install the Vulkan SDK or set VULKAN_SDK.")
endif()

set(EXAMPLE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic/14_frame_in_flight")
set(SHADER_BINARY "${EXAMPLE_OUTPUT_DIRECTORY}/slang.spv")
add_custom_command(
    OUTPUT "${SHADER_BINARY}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${EXAMPLE_OUTPUT_DIRECTORY}"
    COMMAND "${SLANGC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.slang"
            -target spirv -profile spirv_1_4 -emit-spirv-directly
            -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o "${SHADER_BINARY}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.slang"
    VERBATIM)

add_executable(frames_in_flight
    main.cpp src/commandBuffer.cpp src/graphicsPipeline.cpp src/device.cpp src/swapchain.cpp
    src/vulkan_instance.cpp src/window.cpp "${SHADER_BINARY}")
set_target_properties(frames_in_flight PROPERTIES OUTPUT_NAME "14_frames_in_flight" RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIRECTORY}")
target_compile_features(frames_in_flight PRIVATE cxx_std_20)
target_include_directories(frames_in_flight PRIVATE include "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(frames_in_flight PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
if(VULKAN_ENABLE_VALIDATION)
    target_compile_definitions(frames_in_flight PRIVATE VULKAN_ENABLE_VALIDATION)
endif()
target_link_libraries(frames_in_flight PRIVATE glfw Vulkan::Vulkan)
if(MSVC)
    target_compile_options(frames_in_flight PRIVATE /W4 /permissive-)
else()
    target_compile_options(frames_in_flight PRIVATE -Wall -Wextra -Wpedantic)
endif()
