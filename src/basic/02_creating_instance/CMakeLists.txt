find_package(Vulkan REQUIRED)

add_executable(creating_instance main.cpp)

target_compile_features(creating_instance PRIVATE cxx_std_20)

target_include_directories(creating_instance PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(creating_instance PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(creating_instance PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(creating_instance PRIVATE /W4 /permissive-)
else()
    target_compile_options(creating_instance PRIVATE -Wall -Wextra -Wpedantic)
endif()
