#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "device.hpp"
#include "window.hpp"

/**
 * @brief Owns the swap chain and image views used for presentation.
 *
 * The swap chain depends on the selected device, presentation surface,
 * and window framebuffer dimensions.
 */
class Swapchain
{
public:
    /** @brief Creates a swap chain using the best supported surface options. */
    Swapchain(const Device&, const vk::raii::SurfaceKHR&, const Window&);
    vk::Format imageFormat() const { return _imageFormat; }

private:
    struct Support { vk::SurfaceCapabilitiesKHR capabilities; std::vector<vk::SurfaceFormatKHR> formats; std::vector<vk::PresentModeKHR> modes; };
    Support query() const;

    vk::SurfaceFormatKHR chooseFormat(const std::vector<vk::SurfaceFormatKHR>&) const;
    vk::PresentModeKHR chooseMode(const std::vector<vk::PresentModeKHR>&) const;
    vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR&) const;

    const Device& _device;
    const vk::raii::SurfaceKHR& _surface;
    const Window& _window;

    vk::raii::SwapchainKHR _swapchain = nullptr;
    std::vector<vk::raii::ImageView> _imageViews;
    vk::Format _imageFormat = vk::Format::eUndefined;
};
