find_package(Vulkan REQUIRED)

add_executable(validation_layer main.cpp)

target_compile_features(validation_layer PRIVATE cxx_std_20)

target_include_directories(validation_layer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(validation_layer PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(validation_layer PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(validation_layer PRIVATE /W4 /permissive-)
else()
    target_compile_options(validation_layer PRIVATE -Wall -Wextra -Wpedantic)
endif()
