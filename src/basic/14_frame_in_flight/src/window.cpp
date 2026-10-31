#include "window.hpp"

#include <stdexcept>

Window::Window(uint32_t width, uint32_t height, const char* title)
{
    // GLFW must be initialized before creating a native window.
    if (glfwInit() == GLFW_FALSE)
        throw std::runtime_error("failed to initialize GLFW");

    // Vulkan owns presentation, so GLFW must not create an OpenGL context.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title, nullptr, nullptr);
    if (_window == nullptr)
    {
        glfwTerminate();
        throw std::runtime_error("failed to create GLFW window");
    }
}

Window::~Window()
{
    // Destroy the window before terminating GLFW.
    if (_window != nullptr)
        glfwDestroyWindow(_window);

    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(_window) != 0;
}

void Window::pollEvents() const
{
    glfwPollEvents();
}

void Window::framebufferSize(int& width, int& height) const
{
    // Framebuffer pixels are required for swap-chain extent selection on high-DPI displays.
    glfwGetFramebufferSize(_window, &width, &height);
}
