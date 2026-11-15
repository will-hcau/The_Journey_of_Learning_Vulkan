#include "vulkan_instance.hpp"

#include <cstring>
#include <ranges>
#include <stdexcept>

#include "log.h"
#include "vulkan_config.hpp"

VulkanInstance::VulkanInstance()
{
    // Validation layers must be available before they can be requested by the instance.
#ifdef VULKAN_ENABLE_VALIDATION
    checkValidationLayerSupport();
#endif

    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = vulkan_config::applicationName,
        .applicationVersion = vulkan_config::applicationVersion,
        .pEngineName = vulkan_config::engineName,
        .engineVersion = vulkan_config::engineVersion,
        .apiVersion = vulkan_config::apiVersion};

    const auto extensions = requiredExtensions();
    const auto available = _context.enumerateInstanceExtensionProperties();

    // Check extensions explicitly so initialization failures explain the missing capability.
    for (const char* required : extensions)
    {
        if (std::ranges::none_of(available, [required](const auto& e) {
                return std::strcmp(e.extensionName, required) == 0;
            }))
            throw std::runtime_error("required Vulkan instance extension is unavailable: " + std::string(required));
    }

#ifdef VULKAN_ENABLE_VALIDATION
    constexpr auto layers = vulkan_config::validationLayers;
#else
    constexpr std::array<const char*, 0> layers{};
#endif

    // The instance is the root Vulkan object. Physical-device enumeration and
    // instance-level debug facilities depend on it.
    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()};

#ifdef __APPLE__
    createInfo.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    _instance = vk::raii::Instance(_context, createInfo);

#ifdef VULKAN_ENABLE_VALIDATION
    createDebugMessenger();
#endif

#ifdef VULKAN_ENABLE_VALIDATION
    constexpr const char* validationState = "enabled";
#else
    constexpr const char* validationState = "disabled";
#endif

    LOG_INFO("Vulkan instance created with validation layers " << validationState << '.');
}

std::vector<const char*> VulkanInstance::requiredExtensions() const
{
    // GLFW supplies the platform-specific extensions needed for surface creation.
    uint32_t count = 0;
    const char* const* glfw = glfwGetRequiredInstanceExtensions(&count);
    if (glfw == nullptr)
        throw std::runtime_error("failed to find GLFW Vulkan extensions");

    std::vector<const char*> extensions(glfw, glfw + count);

    extensions.insert(
        extensions.end(),
        vulkan_config::requiredInstanceExtensions.begin(),
        vulkan_config::requiredInstanceExtensions.end());

#ifdef VULKAN_ENABLE_VALIDATION
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
#endif

#ifdef __APPLE__
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

    return extensions;
}

void VulkanInstance::checkValidationLayerSupport() const
{
    const auto available = _context.enumerateInstanceLayerProperties();

    // A validation-enabled build should fail early if the SDK layer is missing.
    for (const char* required : vulkan_config::validationLayers)
        if (std::ranges::none_of(available, [required](const auto& layer) {
                return std::strcmp(layer.layerName, required) == 0;
            }))
            throw std::runtime_error("validation layer is unavailable: " + std::string(required));
}

vk::raii::SurfaceKHR VulkanInstance::createSurface(const Window& window) const
{
    // GLFW creates the native surface handle; vk::raii takes ownership of it.
    VkSurfaceKHR raw = VK_NULL_HANDLE;

    if (glfwCreateWindowSurface(*_instance, window.handle(), nullptr, &raw) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface");

    LOG_INFO("Window surface created.");

    return vk::raii::SurfaceKHR(_instance, raw);
}

#ifdef VULKAN_ENABLE_VALIDATION
VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanInstance::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* data, void*)
{
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
        severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        LOG_ERROR("[validation] " << vk::to_string(type) << ": " << data->pMessage);
    return vk::False;
}

void VulkanInstance::createDebugMessenger()
{
    // The debug messenger reports validation warnings and errors while the instance lives.
    const vk::DebugUtilsMessengerCreateInfoEXT info{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &debugCallback};

    _debugMessenger = _instance.createDebugUtilsMessengerEXT(info);
}
#endif
