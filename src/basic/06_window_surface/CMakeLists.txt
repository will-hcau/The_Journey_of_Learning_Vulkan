find_package(Vulkan REQUIRED)

add_executable(window_surface main.cpp)

set_target_properties(window_surface PROPERTIES
    OUTPUT_NAME "06_window_surface"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic"
)

target_compile_features(window_surface PRIVATE cxx_std_20)

target_include_directories(window_surface PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(window_surface PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(window_surface PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(window_surface PRIVATE /W4 /permissive-)
else()
    target_compile_options(window_surface PRIVATE -Wall -Wextra -Wpedantic)
endif()
