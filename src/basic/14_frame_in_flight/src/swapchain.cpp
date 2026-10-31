#include "swapchain.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>

#include "log.h"

Swapchain::Swapchain(const Device& device, const vk::raii::SurfaceKHR& surface, const Window& window)
    : _device(device), _surface(surface), _window(window)
{
    // Query the surface before selecting its format, extent, present mode, and image count.
    const auto support = query();
    if (support.formats.empty() || support.modes.empty())
        throw std::runtime_error("surface does not support a usable swap chain");

    const auto format = chooseFormat(support.formats);
    const auto extent = chooseExtent(support.capabilities);
    _imageFormat = format.format;
    _extent = extent;

    uint32_t count = std::max(3u, support.capabilities.minImageCount);
    if (support.capabilities.maxImageCount != 0)
        count = std::min(count, support.capabilities.maxImageCount);

    const vk::SwapchainCreateInfoKHR info{
        .surface = *_surface,
        .minImageCount = count,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = support.capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseMode(support.modes),
        .clipped = vk::True};

    // The swap chain owns the images that will eventually be presented.
    _swapchain = vk::raii::SwapchainKHR(_device.logicalDevice(), info);

    _images = _swapchain.getImages();

    LOG_INFO("Swap chain created with " << _images.size() << " images.");

    for (const vk::Image image : _images)
    {
        // Each swap-chain image needs a view before it can be used as a render target.
        const vk::ImageViewCreateInfo viewInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format.format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .layerCount = 1}};

        _imageViews.emplace_back(_device.logicalDevice(), viewInfo);
    }

    LOG_INFO("Created " << _imageViews.size() << " swap-chain image views.");
}

Swapchain::Support Swapchain::query() const
{
    return {_device.physicalDevice().getSurfaceCapabilitiesKHR(*_surface),
            _device.physicalDevice().getSurfaceFormatsKHR(*_surface),
            _device.physicalDevice().getSurfacePresentModesKHR(*_surface)};
}

vk::SurfaceFormatKHR Swapchain::chooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const
{
    // Prefer the common sRGB format, but fall back to the first supported format.
    const auto found = std::ranges::find_if(formats, [](const auto& f) {
        return f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return found == formats.end() ? formats.front() : *found;
}

vk::PresentModeKHR Swapchain::chooseMode(const std::vector<vk::PresentModeKHR>& modes) const
{
    // Mailbox reduces tearing when available; FIFO is guaranteed by Vulkan.
    return std::ranges::find(modes, vk::PresentModeKHR::eMailbox) != modes.end()
        ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
    // Use the platform-selected extent when available; otherwise clamp the framebuffer size.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width = 0, height = 0;

    _window.framebufferSize(width, height);

    return {std::clamp<uint32_t>(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}
