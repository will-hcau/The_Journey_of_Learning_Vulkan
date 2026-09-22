find_package(Vulkan REQUIRED)

add_executable(logical_device main.cpp)

target_compile_features(logical_device PRIVATE cxx_std_20)

target_include_directories(logical_device PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(logical_device PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(logical_device PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(logical_device PRIVATE /W4 /permissive-)
else()
    target_compile_options(logical_device PRIVATE -Wall -Wextra -Wpedantic)
endif()
