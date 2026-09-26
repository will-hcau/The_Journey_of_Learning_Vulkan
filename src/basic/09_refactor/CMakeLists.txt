find_package(Vulkan REQUIRED)

option(VULKAN_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)

add_executable(refactor
    main.cpp
    src/window.cpp
    src/vulkan_instance.cpp
    src/device.cpp
    src/swapchain.cpp
)

set_target_properties(refactor PROPERTIES
    OUTPUT_NAME "09_refactor"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic"
)

target_compile_features(refactor PRIVATE cxx_std_20)
target_include_directories(refactor PRIVATE include ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(refactor PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

if(VULKAN_ENABLE_VALIDATION)
    target_compile_definitions(refactor PRIVATE VULKAN_ENABLE_VALIDATION)
endif()

target_link_libraries(refactor PRIVATE glfw Vulkan::Vulkan)

if(MSVC)
    target_compile_options(refactor PRIVATE /W4 /permissive-)
else()
    target_compile_options(refactor PRIVATE -Wall -Wextra -Wpedantic)
endif()
