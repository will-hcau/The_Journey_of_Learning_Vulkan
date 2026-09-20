#include <cstdlib>
#include <cstring>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

#include "log.h"

#include <vulkan/vulkan_raii.hpp> // Vulkan-Hpp RAII interface.

#define GLFW_INCLUDE_NONE // GLFW does not need to include the C Vulkan headers.
#include <GLFW/glfw3.h>

namespace
{
// These values are used when creating the application window.
constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;
}

class SimpleTriangle
{
public:
    void run()
    {
        // Set up the window and Vulkan instance before entering the event loop.
        initWindow();
        initVulkan();
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

    void initVulkan()
    {
        createInstance();
    }

    void createInstance()
    {
        // Describe the application to the Vulkan driver.
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Example Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14};

        // GLFW tells us which instance extensions are needed to present to a window.
        uint32_t glfwExtensionCount = 0;
        const char* const* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (glfwExtensions == nullptr)
            throw std::runtime_error("failed to find GLFW Vulkan extensions");

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef __APPLE__
        // MoltenVK requires this extension and flag on macOS.
        extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

        // Check that every required extension is supported by Vulkan.
        const auto extensionProperties = context.enumerateInstanceExtensionProperties();

        LOG_INFO("Available Vulkan instance extensions");
        for (const auto& extension : extensionProperties)
        {
            LOG_INFO("   " << extension.extensionName);
        }

        for (const char* extensionName : extensions)
        {
            if (std::ranges::none_of(extensionProperties,
                    [extensionName](const auto& extensionProperty)
                    {
                        return std::strcmp(extensionProperty.extensionName, extensionName) == 0;
                    }))
            {
                throw std::runtime_error(
                    "Required Vulkan extension not supported: " + std::string(extensionName));
            }
        }

        // The RAII instance automatically destroys the Vulkan instance when it goes out of scope.
#ifdef __APPLE__
        const vk::InstanceCreateInfo createInfo{
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()};
#else
        const vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()};
#endif

        instance = vk::raii::Instance(context, createInfo);
        LOG_INFO("Vulkan instance created successfully.");
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
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
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
        LOG_ERROR(exception.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
