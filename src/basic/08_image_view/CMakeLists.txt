find_package(Vulkan REQUIRED)

add_executable(image_view main.cpp)

set_target_properties(image_view PROPERTIES
    OUTPUT_NAME "08_image_view"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/basic"
)

target_compile_features(image_view PRIVATE cxx_std_20)

target_include_directories(image_view PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(image_view PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(image_view PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(image_view PRIVATE /W4 /permissive-)
else()
    target_compile_options(image_view PRIVATE -Wall -Wextra -Wpedantic)
endif()
