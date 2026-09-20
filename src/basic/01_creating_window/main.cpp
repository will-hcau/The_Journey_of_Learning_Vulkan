#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan_raii.hpp> // Vulkan-Hpp RAII interface.

#define GLFW_INCLUDE_NONE // GLFW does not need to include the C Vulkan headers.
#include <GLFW/glfw3.h>

namespace {
// These values are used when creating the application window.
constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;
}

class SimpleTriangle
{
public:

    void run()
    {
        // Set up the window, keep it alive until it closes, then release it.
        initWindow();
        mainLoop();
        cleanup();
    }

private:

    void initWindow()
    {
        // GLFW must be initialized before any other GLFW function is called.
        if (glfwInit() == GLFW_FALSE)
        {
            throw std::runtime_error("failed to initialize GLFW");
        }

        // Vulkan creates its own rendering surface, so GLFW must not create an OpenGL context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Resizing will be handled in a later tutorial step.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(kWidth, kHeight, "Vulkan Triangle", nullptr, nullptr);
        if (window == nullptr)
        {
            glfwTerminate();
            throw std::runtime_error("failed to create GLFW window");
        }
    }

    void mainLoop()
    {
        // Process window events until the user closes the window.
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        // Destroy the window before shutting down the GLFW library.
        if (window != nullptr)
        {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    GLFWwindow* window = nullptr;
};

int main()
{
    try
    {
        SimpleTriangle app;
        app.run();
    }
    catch (const std::exception& exception)
    {
        // Report initialization or runtime errors and return a failure status.
        std::cerr << exception.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
