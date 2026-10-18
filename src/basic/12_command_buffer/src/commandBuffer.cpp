#include "commandBuffer.hpp"

#include <array>
#include <stdexcept>
#include <utility>

#include "log.h"

CommandBuffer::CommandBuffer(const Device& device)
    : _device(device)
{
    // Command buffers are submitted to the graphics queue, so the pool must
    // use the queue family selected for graphics and presentation.
    const vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = *_device.queueFamilies().graphicsAndPresent};

    _pool = vk::raii::CommandPool(_device.logicalDevice(), poolInfo);

    // Allocate one primary buffer. Primary buffers can be submitted directly
    // to a queue; recording draw commands comes in the following chapter.
    const vk::CommandBufferAllocateInfo allocationInfo{
        .commandPool = *_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};

    auto buffers = vk::raii::CommandBuffers(_device.logicalDevice(), allocationInfo);
    _buffer = std::move(buffers.front());

    LOG_INFO("Command pool and primary command buffer created.");
}

void CommandBuffer::record(
    uint32_t imageIndex,
    const Swapchain& swapchain,
    const GraphicsPipeline& graphicsPipeline)
{
    if (imageIndex >= swapchain.images().size())
        throw std::out_of_range("swap-chain image index is out of range");

    // Begin recording a fresh command sequence for this swap-chain image.
    _buffer.begin({});

    const auto transitionImage = [this, &swapchain, imageIndex](
                                     vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout,
                                     vk::AccessFlags2 sourceAccess,
                                     vk::AccessFlags2 destinationAccess,
                                     vk::PipelineStageFlags2 sourceStage,
                                     vk::PipelineStageFlags2 destinationStage) {
        // Synchronize access and change the image layout in one barrier.
        const vk::ImageMemoryBarrier2 barrier{
            .srcStageMask = sourceStage,
            .srcAccessMask = sourceAccess,
            .dstStageMask = destinationStage,
            .dstAccessMask = destinationAccess,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchain.images()[imageIndex],
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .layerCount = 1}};

        const vk::DependencyInfo dependencyInfo{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier};
        _buffer.pipelineBarrier2(dependencyInfo);
    };

    // Prepare the swap-chain image for use as a color attachment.
    transitionImage(
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {}, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    vk::ClearValue clearColor;
    clearColor.color = vk::ClearColorValue{std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}};

    const vk::RenderingAttachmentInfo colorAttachment{
        .imageView = *swapchain.imageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor};

    const vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = swapchain.extent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment};

    _buffer.beginRendering(renderingInfo);

    // Bind the pipeline and dynamic viewport state before drawing.
    _buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline.pipeline());
    _buffer.setViewport(0, vk::Viewport{
                                0.0F, 0.0F,
                                static_cast<float>(swapchain.extent().width),
                                static_cast<float>(swapchain.extent().height),
                                0.0F, 1.0F});
    _buffer.setScissor(0, vk::Rect2D{{0, 0}, swapchain.extent()});
    _buffer.draw(3, 1, 0, 0);

    _buffer.endRendering();

    // Make the rendered image ready for presentation.
    transitionImage(
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite, {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe);

    _buffer.end();
}
