cmake_minimum_required(VERSION 3.24)

project(TheJourneyOfLearningVulkan VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# CMake cannot safely open an interactive prompt during a normal configure.
# The cache entry is therefore the first-time "question"; it is remembered
# in CMakeCache.txt after the first configure.
set(VULKAN_SDK_PATH "" CACHE PATH
    "Path to the Vulkan SDK installation (for example /path/to/1.4.xxx)")
if(NOT VULKAN_SDK_PATH AND DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}" CACHE PATH
        "Path to the Vulkan SDK installation" FORCE)
endif()

if(VULKAN_SDK_PATH)
    set(Vulkan_ROOT "${VULKAN_SDK_PATH}")
    list(APPEND CMAKE_PREFIX_PATH "${VULKAN_SDK_PATH}")
endif()

# Build all tutorial dependencies from source and keep them out of the
# repository. They are downloaded into CMake's dependency directory.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_Declare(tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw glm tinyobjloader)

add_subdirectory(src)
