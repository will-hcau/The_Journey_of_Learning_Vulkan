# The Journey of Learning Vulkan

This is a CMake-based educational project containing small C++20 examples for learning Vulkan, from basic setup through instance creation, validation layers, swapchains, pipelines, and rendering.

## Requirements

- CMake 3.24 or newer
- A C++20 compiler
- Git
- Vulkan SDK (recommended for Vulkan examples)

The project downloads and builds GLFW, GLM, and tinyobjloader from source using CMake's `FetchContent`. No separate installation of these libraries is required.

## Setup and build

Configure the project from its root directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

If the `VULKAN_SDK` environment variable is not already set, provide the SDK path during configuration:

```bash
cmake -S . -B build \
  -DVULKAN_SDK_PATH=/path/to/vulkan-sdk \
  -DCMAKE_BUILD_TYPE=Debug
```

The SDK path is saved in `build/CMakeCache.txt`, so it only needs to be specified once for a build directory.

Build the whole project with:

```bash
cmake --build build --parallel
```

The first build downloads and compiles the third-party libraries, so it may take a little longer than later builds.

## Build one project

Each example is a separate CMake target. To build only the hello-world example:

```bash
cmake --build build --target hello_world
```

Run it with:

```bash
./build/src/basic/00_hello_world/hello_world
```

To see all available CMake targets:

```bash
cmake --build build --target help
```

## Available projects

| Project | Target | Description |
| --- | --- | --- |
| `src/basic/00_hello_world` | `hello_world` | Minimal C++20 program that verifies the project builds. |

New examples will be added under `src`, ordered by number. For example:

```text
src/
└── basic/
    ├── 00_hello_world/
    ├── 01_instance_creation/
    └── 02_validation_layers/
```
