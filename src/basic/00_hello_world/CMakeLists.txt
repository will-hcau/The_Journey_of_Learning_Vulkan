add_executable(hello_world main.cpp)

target_compile_features(hello_world PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(hello_world PRIVATE /W4 /permissive-)
else()
    target_compile_options(hello_world PRIVATE -Wall -Wextra -Wpedantic)
endif()
