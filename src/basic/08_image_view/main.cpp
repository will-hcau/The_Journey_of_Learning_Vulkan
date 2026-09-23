#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
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
constexpr const char* kPortabilitySubsetExtension = "VK_KHR_portability_subset";

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
    struct SwapChainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapChainSupportDetails _querySwapChainSupport(const vk::raii::PhysicalDevice& candidate) const
    {
        // These three queries describe what this surface can actually present.
        return {
            candidate.getSurfaceCapabilitiesKHR(*surface),
            candidate.getSurfaceFormatsKHR(*surface),
            candidate.getSurfacePresentModesKHR(*surface)};
    }

    vk::SurfaceFormatKHR _chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const
    {
        // Prefer the common 8-bit sRGB format, but accept any supported format.
        const auto preferred = std::ranges::find_if(
            formats,
            [](const auto& format)
            {
                return format.format == vk::Format::eB8G8R8A8Srgb &&
                       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            });
        return preferred == formats.end() ? formats.front() : *preferred;
    }

    vk::PresentModeKHR _chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& modes) const
    {
        // Mailbox avoids tearing when available; FIFO is guaranteed by Vulkan.
        const bool mailboxAvailable = std::ranges::find(modes, vk::PresentModeKHR::eMailbox) != modes.end();
        return mailboxAvailable ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D _chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        int width = 0;
        int height = 0;
        // GLFW reports framebuffer pixels, which matters on Retina displays.
        glfwGetFramebufferSize(window, &width, &height);

        return {
            std::clamp<uint32_t>(static_cast<uint32_t>(width),
                                 capabilities.minImageExtent.width,
                                 capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(static_cast<uint32_t>(height),
                                 capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height)};
    }

    void _createImageViews()
    {
        // Each swap-chain image needs a view before it can be used as a render target.
        assert(swapChainImageViews.empty());

        const vk::ImageViewCreateInfo createInfo{
            .viewType = vk::ImageViewType::e2D,
            .format = swapChainSurfaceFormat.format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .layerCount = 1}};

        for (const vk::Image image : swapChainImages)
        {
            auto imageViewCreateInfo = createInfo;
            imageViewCreateInfo.image = image;
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);
        }

        LOG_INFO("Created " << swapChainImageViews.size() << " swap-chain image views.");
    }

    void _createSwapChain()
    {
        const auto support = _querySwapChainSupport(physicalDevice);
        if (support.formats.empty() || support.presentModes.empty())
            throw std::runtime_error("surface does not support a usable swap chain");

        swapChainSurfaceFormat = _chooseSwapSurfaceFormat(support.formats);
        swapChainExtent = _chooseSwapExtent(support.capabilities);

        uint32_t imageCount = std::max(3u, support.capabilities.minImageCount);
        if (support.capabilities.maxImageCount != 0)
            imageCount = std::min(imageCount, support.capabilities.maxImageCount);

        // The swap chain owns the images that will eventually be presented.
        const vk::SwapchainCreateInfoKHR createInfo{
            .surface = *surface,
            .minImageCount = imageCount,
            .imageFormat = swapChainSurfaceFormat.format,
            .imageColorSpace = swapChainSurfaceFormat.colorSpace,
            .imageExtent = swapChainExtent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform = support.capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = _chooseSwapPresentMode(support.presentModes),
            .clipped = vk::True};

        swapChain = vk::raii::SwapchainKHR(device, createInfo);
        swapChainImages = swapChain.getImages();
        LOG_INFO("Swap chain created with " << swapChainImages.size() << " images.");
    }

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
        createSurface();
        _pickPhysicalDevice();
        _createLogicalDevice();
        _createSwapChain();
        _createImageViews();
    }

    uint32_t _findGraphicsAndPresentQueueFamily() const
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        for (uint32_t familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex)
            if ((queueFamilies[familyIndex].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{} &&
                physicalDevice.getSurfaceSupportKHR(familyIndex, *surface))
                return familyIndex;
        throw std::runtime_error("failed to find a queue family for graphics and presentation");
    }

    void _createLogicalDevice()
    {
        // Create one queue from the first family that supports graphics commands.
        const uint32_t graphicsQueueFamily = _findGraphicsAndPresentQueueFamily();
        const auto enabledDeviceExtensions = _getEnabledDeviceExtensions();
        constexpr float queuePriority = 0.5F;
        const vk::DeviceQueueCreateInfo queueCreateInfo{
            .queueFamilyIndex = graphicsQueueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority};

        // Querying features only tells us what is supported. This chain requests
        // the features that this example and later tutorial steps will use.
        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain{
                {},
                {.shaderDrawParameters = true},
                {.dynamicRendering = true},
                {.extendedDynamicState = true}};

        const vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size()),
            .ppEnabledExtensionNames = enabledDeviceExtensions.data()};

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(device, graphicsQueueFamily, 0);

        LOG_INFO("Logical device and graphics queue created.");
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

    bool _supportsPresentationQueue(const vk::raii::PhysicalDevice& candidate) const
    {
        const auto families = candidate.getQueueFamilyProperties();
        for (uint32_t familyIndex = 0; familyIndex < families.size(); ++familyIndex)
            if ((families[familyIndex].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{} &&
                candidate.getSurfaceSupportKHR(familyIndex, *surface))
                return true;
        return false;
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

    std::vector<const char*> _getEnabledDeviceExtensions() const
    {
        const auto available = physicalDevice.enumerateDeviceExtensionProperties();
        std::vector<const char*> extensions = kRequiredDeviceExtensions;

        // MoltenVK requires this extension when the physical device exposes it.
        const bool supportsPortabilitySubset = std::ranges::any_of(available,
            [](const auto& extension)
            {
                return std::strcmp(extension.extensionName, kPortabilitySubsetExtension) == 0;
            });

        if (supportsPortabilitySubset)
            extensions.push_back(kPortabilitySubsetExtension);

        return extensions;
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
               _supportsPresentationQueue(candidate) &&
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

        const auto properties = physicalDevice.getProperties();
        const uint32_t apiVersion = properties.apiVersion;
        LOG_INFO("Selected GPU: " << properties.deviceName);
        LOG_INFO("GPU Vulkan API version: "
                 << VK_VERSION_MAJOR(apiVersion) << '.'
                 << VK_VERSION_MINOR(apiVersion) << '.'
                 << VK_VERSION_PATCH(apiVersion));
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
        void*)
    {
        // Vulkan calls this function for messages matching the filters below.
        const auto message = "[validation layer] type " + vk::to_string(type) +
                             " message: " + std::string(callbackData->pMessage);

        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
            severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            LOG_ERROR(message);
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
        LOG_INFO("Vulkan instance created with validation layers "
                 << (kEnableValidationLayers ? "enabled" : "disabled") << '.');
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

    void createSurface()
    {
        VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface");
        surface = vk::raii::SurfaceKHR(instance, rawSurface);
        LOG_INFO("Window surface created.");
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
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;
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
