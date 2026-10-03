#pragma once

#include <optional>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "vulkan_instance.hpp"

/** @brief Queue family selected for both graphics commands and presentation. */
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsAndPresent;

    bool complete() const { return graphicsAndPresent.has_value(); }
};

/**
 * @brief Selects a suitable physical device and owns its logical device.
 *
 * Suitability includes API version, queue families, required extensions,
 * and the features needed by this chapter.
 */
class Device
{
public:
    /** @brief Selects a GPU and creates its logical device. */
    Device(const VulkanInstance& instance, const vk::raii::SurfaceKHR& surface);
    QueueFamilyIndices queueFamilies() const { return _queueFamilies; }
    const vk::raii::PhysicalDevice& physicalDevice() const { return _physicalDevice; }
    const vk::raii::Device& logicalDevice() const { return _device; }

private:
    QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice&) const;
    bool suitable(const vk::raii::PhysicalDevice&, QueueFamilyIndices&) const;
    bool supportsExtensions(const vk::raii::PhysicalDevice&) const;
    bool supportsFeatures(const vk::raii::PhysicalDevice&) const;
    std::vector<const char*> enabledExtensions() const;

    const vk::raii::SurfaceKHR& _surface;
    vk::raii::PhysicalDevice _physicalDevice = nullptr;
    vk::raii::Device _device = nullptr;
    QueueFamilyIndices _queueFamilies;
};
