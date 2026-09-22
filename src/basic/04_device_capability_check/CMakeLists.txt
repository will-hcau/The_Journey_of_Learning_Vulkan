find_package(Vulkan REQUIRED)

add_executable(device_capability_check main.cpp)

set_target_properties(device_capability_check PROPERTIES
    OUTPUT_NAME "04_device_capability_check"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic"
)

target_compile_features(device_capability_check PRIVATE cxx_std_20)

target_include_directories(device_capability_check PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(device_capability_check PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(device_capability_check PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(device_capability_check PRIVATE /W4 /permissive-)
else()
    target_compile_options(device_capability_check PRIVATE -Wall -Wextra -Wpedantic)
endif()
