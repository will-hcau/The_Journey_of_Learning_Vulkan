#pragma once

#include <cstdint>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/**
 * @brief Owns the GLFW window and its process-wide GLFW lifetime.
 *
 * GLFW is initialized in the constructor and terminated in the destructor.
 * The class is non-copyable because it owns the native window handle.
 */
class Window
{
public:
    /** @brief Creates a non-resizable window without an OpenGL context. */
    Window(uint32_t width, uint32_t height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /** @brief Returns whether the user requested the window to close. */
    bool shouldClose() const;

    /** @brief Processes pending GLFW events. */
    void pollEvents() const;

    /** @brief Returns the drawable framebuffer size in pixels. */
    void framebufferSize(int& width, int& height) const;

    /** @brief Returns the native GLFW handle used by surface creation. */
    GLFWwindow* handle() const { return _window; }

private:
    GLFWwindow* _window = nullptr;
};
