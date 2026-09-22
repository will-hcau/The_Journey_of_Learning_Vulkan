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
const std::vector<const char*> kRequiredDeviceExtensions = {vk::KHRSwapchainExtensionName};

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
        _pickPhysicalDevice();
    }

    // The tutorial requires features introduced by Vulkan 1.3.
    bool _supportsVulkan13(const vk::raii::PhysicalDevice& candidate) const
    {
        return candidate.getProperties().apiVersion >= vk::ApiVersion13;
    }

    bool _supportsGraphicsQueue(const vk::raii::PhysicalDevice& candidate) const
    {
        const auto families = candidate.getQueueFamilyProperties();
        return std::ranges::any_of(families,
            [](const auto& family)
            {
                return (family.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{};
            });
    }

    bool _supportsRequiredExtensions(const vk::raii::PhysicalDevice& candidate) const
    {
        const auto available = candidate.enumerateDeviceExtensionProperties();
        return std::ranges::all_of(kRequiredDeviceExtensions,
            [&available](const char* required)
            {
                return std::ranges::any_of(available,
                    [required](const auto& extension)
                    {
                        return std::strcmp(extension.extensionName, required) == 0;
                    });
            });
    }

    bool _supportsRequiredFeatures(const vk::raii::PhysicalDevice& candidate) const
    {
        // Query all feature structures that will be needed by later tutorial steps.
        const auto features = candidate.template getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

        const auto& vulkan11Features = features.template get<vk::PhysicalDeviceVulkan11Features>();
        const auto& vulkan13Features = features.template get<vk::PhysicalDeviceVulkan13Features>();
        const auto& dynamicStateFeatures = features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

        return vulkan11Features.shaderDrawParameters &&
               vulkan13Features.dynamicRendering &&
               dynamicStateFeatures.extendedDynamicState;
    }

    // A device is usable only when every required capability is available.
    bool _isDeviceSuitable(const vk::raii::PhysicalDevice& candidate) const
    {
        return _supportsVulkan13(candidate) &&
               _supportsGraphicsQueue(candidate) &&
               _supportsRequiredExtensions(candidate) &&
               _supportsRequiredFeatures(candidate);
    }

    void _pickPhysicalDevice()
    {
        const auto devices = instance.enumeratePhysicalDevices();
        if (devices.empty())
            throw std::runtime_error("failed to find GPUs with Vulkan support");

        // Select the first device satisfying all application requirements.
        const auto selected = std::ranges::find_if(devices,
            [this](const auto& candidate)
            {
                return _isDeviceSuitable(candidate);
            });

        if (selected == devices.end())
            throw std::runtime_error("failed to find a suitable GPU");

        physicalDevice = *selected;
        LOG_INFO("Selected GPU: " << physicalDevice.getProperties().deviceName);
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
    vk::raii::PhysicalDevice physicalDevice = nullptr;
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
