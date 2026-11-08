#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "window.hpp"

/**
 * @brief Owns the Vulkan instance and optional validation debug messenger.
 *
 * Instance extensions are assembled here because GLFW contributes
 * platform-specific requirements at runtime.
 */
class VulkanInstance
{
public:
    /** @brief Creates an instance using the configured Vulkan requirements. */
    VulkanInstance();

    /** @brief Returns the RAII instance used by physical-device enumeration. */
    vk::raii::Instance& instance() { return _instance; }
    const vk::raii::Instance& instance() const { return _instance; }

    /** @brief Creates a presentation surface for the supplied GLFW window. */
    vk::raii::SurfaceKHR createSurface(const Window& window) const;

private:
    std::vector<const char*> requiredExtensions() const;
    void checkValidationLayerSupport() const;

#ifdef VULKAN_ENABLE_VALIDATION
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT,
        vk::DebugUtilsMessageTypeFlagsEXT,
        const vk::DebugUtilsMessengerCallbackDataEXT*, void*);

    void createDebugMessenger();
#endif

    vk::raii::Context _context;
    vk::raii::Instance _instance = nullptr;

#ifdef VULKAN_ENABLE_VALIDATION
    // Declare this after the instance so it is destroyed before the instance.
    vk::raii::DebugUtilsMessengerEXT _debugMessenger = nullptr;
#endif
};
