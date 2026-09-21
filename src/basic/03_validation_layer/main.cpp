#include <cstdlib>
#include <cstring>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

#include "log.h"

#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace
{
constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;

// The Khronos validation layer checks Vulkan calls against the specification.
const std::vector<const char*> kValidationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
// Release builds skip diagnostic checks to avoid their runtime overhead.
constexpr bool kEnableValidationLayers = false;
#else
// Debug builds enable validation to make Vulkan mistakes easier to find.
constexpr bool kEnableValidationLayers = true;
#endif
}

class SimpleTriangle
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initWindow()
    {
        if (glfwInit() == GLFW_FALSE)
            throw std::runtime_error("failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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
        setupDebugMessenger();
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
        void*)
    {
        // Vulkan calls this function for messages matching the filters below.
        const auto message = "validation layer: type " + vk::to_string(type) + " message: " + std::string(callbackData->pMessage);

        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        {
            LOG_ERROR(message);
        }
        else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ||
                 severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
        {
            LOG_INFO(message);
        }
        else
        {
            LOG_DEBUG(message);
        }

        return vk::False;
    }

    std::vector<const char*> getRequiredInstanceExtensions() const
    {
        // GLFW supplies platform extensions; debug-utils enables our callback.
        uint32_t glfwExtensionCount = 0;

        const char* const* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (glfwExtensions == nullptr)
            throw std::runtime_error("failed to find GLFW Vulkan extensions");

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (kEnableValidationLayers)
            extensions.push_back(vk::EXTDebugUtilsExtensionName);

#ifdef __APPLE__
        extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif
        return extensions;
    }

    void checkValidationLayerSupport() const
    {
        // Layers must be installed by the Vulkan SDK or another runtime.
        const auto availableLayers = context.enumerateInstanceLayerProperties();

        for (const char* requestedLayer : kValidationLayers)
        {
            const bool found = std::ranges::any_of(availableLayers,
                [requestedLayer](const auto& layer)
                {
                    return std::strcmp(layer.layerName, requestedLayer) == 0;
                });
            if (!found)
                throw std::runtime_error("Required validation layer not supported: " + std::string(requestedLayer));
        }
    }

    void createInstance()
    {
        if (kEnableValidationLayers)
            checkValidationLayerSupport();

        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Example Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14};

        // Check extensions before creating the instance for a useful error.
        const auto extensions = getRequiredInstanceExtensions();
        const auto extensionProperties = context.enumerateInstanceExtensionProperties();

        for (const char* extensionName : extensions)
        {
            if (std::ranges::none_of(extensionProperties,
                    [extensionName](const auto& extension)
                    {
                        return std::strcmp(extension.extensionName, extensionName) == 0;
                    }))
            {
                throw std::runtime_error("Required Vulkan extension not supported: " + std::string(extensionName));
            }
        }

        // Pass layer names to Vulkan only when validation is enabled.
        const std::vector<const char*> enabledLayers =
            kEnableValidationLayers ? kValidationLayers : std::vector<const char*>{};

#ifdef __APPLE__
        const vk::InstanceCreateInfo createInfo{
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
            .ppEnabledLayerNames = enabledLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()};
#else
        const vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
            .ppEnabledLayerNames = enabledLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()};
#endif

        instance = vk::raii::Instance(context, createInfo);
        LOG_INFO("Vulkan instance created with validation layers " << (kEnableValidationLayers ? "enabled" : "disabled") << '.');
    }

    void setupDebugMessenger()
    {
        if (!kEnableValidationLayers)
            return;

        // Request every severity so verbose and informational messages are
        // visible in addition to warnings and errors.
        const vk::DebugUtilsMessengerCreateInfoEXT createInfo{
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = &debugCallback};

        debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
            glfwPollEvents();
    }

    void cleanup()
    {
        if (window != nullptr)
            glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow* window = nullptr;
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
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
        LOG_ERROR(exception.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
