#pragma once

#include <array>
#include <cstdint>

#include <vulkan/vulkan_raii.hpp>

namespace vulkan_config
{
// Application information

/** @brief Initial window width in framebuffer-independent units. */
inline constexpr uint32_t windowWidth = 800;

/** @brief Initial window height in framebuffer-independent units. */
inline constexpr uint32_t windowHeight = 600;

/** @brief Title displayed by the GLFW window. */
inline constexpr const char* applicationName = "Vulkan Triangle";

/** @brief Name reported to Vulkan tooling as the application engine. */
inline constexpr const char* engineName = "Example Engine";

/** @brief Vulkan application version reported during instance creation. */
inline constexpr uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);

/** @brief Engine version reported during instance creation. */
inline constexpr uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);

/** @brief Vulkan API version required by this chapter. */
inline constexpr uint32_t apiVersion = vk::ApiVersion14;

// Instance and device extension requirements

inline constexpr std::array<const char*, 0> requiredInstanceExtensions = {};

inline constexpr std::array<const char*, 1> requiredDeviceExtensions = {
    vk::KHRSwapchainExtensionName};

// Device feature requirements

inline constexpr bool requireShaderDrawParameters = true;
inline constexpr bool requireDynamicRendering = true;
inline constexpr bool requireExtendedDynamicState = true;

// Validation configuration

#ifdef VULKAN_ENABLE_VALIDATION
inline constexpr std::array<const char*, 1> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
#endif
}
