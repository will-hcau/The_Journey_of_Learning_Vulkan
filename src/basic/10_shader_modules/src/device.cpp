#include "device.hpp"

#include <cstring>
#include <ranges>
#include <stdexcept>

#include "log.h"
#include "vulkan_config.hpp"

Device::Device(const VulkanInstance& instance, const vk::raii::SurfaceKHR& surface)
    : _surface(surface)
{
    // Select the first physical device satisfying every requirement of this chapter.
    const auto devices = instance.instance().enumeratePhysicalDevices();
    for (const auto& candidate : devices)
    {
        QueueFamilyIndices indices;
        if (suitable(candidate, indices))
        {
            _physicalDevice = candidate;
            _queueFamilies = indices;
            break;
        }
    }

    if (*_physicalDevice == vk::PhysicalDevice{})
        throw std::runtime_error("failed to find a suitable GPU");

    // This example uses one queue that supports both graphics and presentation.
    const float priority = 0.5F;

    const vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = *_queueFamilies.graphicsAndPresent,
        .queueCount = 1,
        .pQueuePriorities = &priority};

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> features{
        {},
        {.shaderDrawParameters = vulkan_config::requireShaderDrawParameters},
        {.dynamicRendering = vulkan_config::requireDynamicRendering},
        {.extendedDynamicState = vulkan_config::requireExtendedDynamicState}};

    const auto extensions = enabledExtensions();

    // Request only features verified during physical-device selection.
    const vk::DeviceCreateInfo info{
        .pNext = &features.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()};

    _device = vk::raii::Device(_physicalDevice, info);

    const auto properties = _physicalDevice.getProperties();
    const uint32_t apiVersion = properties.apiVersion;

    LOG_INFO("Selected GPU: " << properties.deviceName);
    LOG_INFO("GPU Vulkan API version: "
             << VK_VERSION_MAJOR(apiVersion) << '.'
             << VK_VERSION_MINOR(apiVersion) << '.'
             << VK_VERSION_PATCH(apiVersion));
    LOG_INFO("Logical device and graphics queue created.");
}

QueueFamilyIndices Device::findQueueFamilies(const vk::raii::PhysicalDevice& device) const
{
    QueueFamilyIndices result;
    const auto families = device.getQueueFamilyProperties();

    // Select the first queue family that supports both operations.
    for (uint32_t i = 0; i < families.size(); ++i)
    {
        const bool supportsGraphics = (families[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{};
        const bool supportsPresentation = device.getSurfaceSupportKHR(i, *_surface);

        if (supportsGraphics && supportsPresentation)
        {
            result.graphicsAndPresent = i;
            break;
        }
    }

    return result;
}

bool Device::supportsExtensions(const vk::raii::PhysicalDevice& device) const
{
    const auto available = device.enumerateDeviceExtensionProperties();

    return std::ranges::all_of(vulkan_config::requiredDeviceExtensions, [&available](const char* required) {
        return std::ranges::any_of(available, [required](const auto& extension) {
            return std::strcmp(extension.extensionName, required) == 0;
        });
    });
}

bool Device::supportsFeatures(const vk::raii::PhysicalDevice& device) const
{
    // Query the feature chain before enabling it on the logical device.
    const auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    const auto& vulkan11Features = features.template get<vk::PhysicalDeviceVulkan11Features>();
    const auto& vulkan13Features = features.template get<vk::PhysicalDeviceVulkan13Features>();
    const auto& dynamicStateFeatures = features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    return (!vulkan_config::requireShaderDrawParameters || vulkan11Features.shaderDrawParameters) &&
           (!vulkan_config::requireDynamicRendering || vulkan13Features.dynamicRendering) &&
           (!vulkan_config::requireExtendedDynamicState || dynamicStateFeatures.extendedDynamicState);
}

bool Device::suitable(const vk::raii::PhysicalDevice& device, QueueFamilyIndices& indices) const
{
    indices = findQueueFamilies(device);

    // A GPU is usable only when all application requirements are satisfied.
    return device.getProperties().apiVersion >= vulkan_config::apiVersion &&
           indices.complete() &&
           supportsExtensions(device) &&
           supportsFeatures(device);
}

std::vector<const char*> Device::enabledExtensions() const
{
    std::vector<const char*> result(vulkan_config::requiredDeviceExtensions.begin(),
                                    vulkan_config::requiredDeviceExtensions.end());

    const auto available = _physicalDevice.enumerateDeviceExtensionProperties();
    constexpr const char* portability = "VK_KHR_portability_subset";

    if (std::ranges::any_of(available, [portability](const auto& e) {
            return std::strcmp(e.extensionName, portability) == 0;
        }))
        result.push_back(portability);

    return result;
}
