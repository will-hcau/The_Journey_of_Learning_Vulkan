#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "device.hpp"
#include "graphicsPipeline.hpp"
#include "swapchain.hpp"

/** Owns a resettable command pool and one primary command buffer per frame slot. */
class CommandBuffer
{
public:
    explicit CommandBuffer(const Device&, uint32_t framesInFlight);

    /** Record a render pass that draws to the acquired swap-chain image. */
    void record(uint32_t frameIndex, uint32_t imageIndex, const Swapchain&, const GraphicsPipeline&);

    const vk::raii::CommandBuffer& buffer(uint32_t frameIndex) const;

private:
    const Device& _device;
    vk::raii::CommandPool _pool = nullptr;
    std::vector<vk::raii::CommandBuffer> _buffers;
};
