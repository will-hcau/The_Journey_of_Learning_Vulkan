find_package(Vulkan REQUIRED)

add_executable(creating_window main.cpp)

target_compile_features(creating_window PRIVATE cxx_std_20)

target_link_libraries(creating_window PRIVATE
    glfw
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(creating_window PRIVATE /W4 /permissive-)
else()
    target_compile_options(creating_window PRIVATE -Wall -Wextra -Wpedantic)
endif()
