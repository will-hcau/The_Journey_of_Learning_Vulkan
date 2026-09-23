find_package(Vulkan REQUIRED)

add_executable(swapchain main.cpp)

set_target_properties(swapchain PROPERTIES
    OUTPUT_NAME "07_swapchain"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic"
)

target_compile_features(swapchain PRIVATE cxx_std_20)

target_include_directories(swapchain PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(swapchain PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(swapchain PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(swapchain PRIVATE /W4 /permissive-)
else()
    target_compile_options(swapchain PRIVATE -Wall -Wextra -Wpedantic)
endif()
