#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "device.hpp"
#include "graphicsPipeline.hpp"
#include "swapchain.hpp"

/**
 * @brief Owns the command pool and primary command buffer used by the example.
 *
 * The pool is tied to the graphics queue family selected by Device. The
 * command buffer is allocated from that pool and is released automatically
 * when the pool is destroyed.
 */
class CommandBuffer
{
public:
    /** @brief Creates a resettable command pool and one primary command buffer. */
    explicit CommandBuffer(const Device&);

    /**
     * @brief Records the rendering commands for one swap-chain image.
     * @param imageIndex Index of the swap-chain image and image view to render to.
     * @param swapchain Swap-chain resources used as the color attachment.
     * @param graphicsPipeline Graphics pipeline bound for the triangle draw.
     */
    void record(uint32_t imageIndex, const Swapchain&, const GraphicsPipeline&);

    /** @brief Returns the allocated primary command buffer. */
    const vk::raii::CommandBuffer& buffer() const { return _buffer; }

private:
    const Device& _device;
    vk::raii::CommandPool _pool = nullptr;
    vk::raii::CommandBuffer _buffer = nullptr;
};
