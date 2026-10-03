# Dependencies

find_package(Vulkan REQUIRED)

option(VULKAN_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)

# The Vulkan SDK provides slangc, which compiles Slang into SPIR-V.
find_program(
    SLANGC_EXECUTABLE
    NAMES slangc
    HINTS
        "$ENV{VULKAN_SDK}/bin"
        "${Vulkan_ROOT}/bin"
)

if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc was not found. Install the Vulkan SDK or set VULKAN_SDK.")
endif()

# Shader compilation
set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shader/shader.slang")

# Keep the shader binary beside the example executable inside this example's
# own output directory. The application resolves this location from argv[0].
set(EXAMPLE_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/src/basic/10_shader_modules"
)
set(SHADER_BINARY "${EXAMPLE_OUTPUT_DIRECTORY}/slang.spv")

add_custom_command(
    OUTPUT "${SHADER_BINARY}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${EXAMPLE_OUTPUT_DIRECTORY}"
    COMMAND "${SLANGC_EXECUTABLE}" "${SHADER_SOURCE}"
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain
            -entry fragMain
            -o "${SHADER_BINARY}"
    DEPENDS "${SHADER_SOURCE}"
    COMMENT "Compiling Slang shader to SPIR-V"
    VERBATIM
)

# Example sources
set(EXAMPLE_SOURCES
    main.cpp
    src/device.cpp
    src/graphicsPipeline.cpp
    src/swapchain.cpp
    src/vulkan_instance.cpp
    src/window.cpp
)

add_executable(
    shader_modules
    ${EXAMPLE_SOURCES}
    "${SHADER_BINARY}"
)

set_target_properties(
    shader_modules
    PROPERTIES
        OUTPUT_NAME "10_shader_modules"
        RUNTIME_OUTPUT_DIRECTORY "${EXAMPLE_OUTPUT_DIRECTORY}"
)

# Compiler and linker configuration
target_compile_features(
    shader_modules
    PRIVATE cxx_std_20
)

target_include_directories(
    shader_modules
    PRIVATE
        include
        "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(shader_modules PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

if(VULKAN_ENABLE_VALIDATION)
    target_compile_definitions(shader_modules PRIVATE VULKAN_ENABLE_VALIDATION)
endif()

target_link_libraries(
    shader_modules
    PRIVATE
        glfw
        Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(shader_modules PRIVATE /W4 /permissive-)
else()
    target_compile_options(shader_modules PRIVATE -Wall -Wextra -Wpedantic)
endif()
