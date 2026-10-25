#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

#include "commandBuffer.hpp"
#include "device.hpp"
#include "graphicsPipeline.hpp"
#include "log.h"
#include "swapchain.hpp"
#include "vulkan_config.hpp"
#include "vulkan_instance.hpp"
#include "window.hpp"

class GraphicPipelineExample
{
public:
    void setup(const std::filesystem::path& directory)
    {
        // Create Vulkan objects in dependency order. Each later object uses
        // resources owned by the objects created before it.
        _window = std::make_unique<Window>(vulkan_config::windowWidth, vulkan_config::windowHeight, vulkan_config::applicationName);
        _instance = std::make_unique<VulkanInstance>();
        _surface = std::make_unique<vk::raii::SurfaceKHR>(_instance->createSurface(*_window));
        _device = std::make_unique<Device>(*_instance, *_surface);
        _swapchain = std::make_unique<Swapchain>(*_device, *_surface, *_window);
        _pipeline = std::make_unique<GraphicsPipeline>(*_device, *_swapchain, directory);
        _commandBuffer = std::make_unique<CommandBuffer>(*_device);

        // This semaphore is signaled when an acquired image is ready to render.
        _imageAvailable = vk::raii::Semaphore(
            _device->logicalDevice(), vk::SemaphoreCreateInfo{});

        // Presentation can still be using a previous render-finished semaphore
        // after the graphics fence is signaled. Use one semaphore per image so
        // that a semaphore is not reused while presentation still waits on it.
        for (std::size_t image = 0; image < _swapchain->images().size(); ++image)
            _renderFinished.emplace_back(_device->logicalDevice(), vk::SemaphoreCreateInfo{});

        // The fence lets the CPU wait until the previous frame has finished
        // before reusing the command buffer and synchronization objects.
        _drawFence = vk::raii::Fence(
            _device->logicalDevice(),
            vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }

    void run()
    {
        // Each iteration processes window events and renders one frame.
        while (!_window->shouldClose())
        {
            _window->pollEvents();
            drawFrame();
        }

        _device->logicalDevice().waitIdle();
    }

private:
    void drawFrame()
    {
        // Wait for the GPU to finish the previous submission. The fence starts
        // signaled so that the first frame does not wait forever.
        if (_device->logicalDevice().waitForFences(*_drawFence, vk::True, UINT64_MAX) != vk::Result::eSuccess)
            throw std::runtime_error("failed to wait for draw fence");

        _device->logicalDevice().resetFences(*_drawFence);

        // Ask the presentation engine for an available swap-chain image. The
        // imageAvailable semaphore is signaled when that image can be rendered.
        auto [acquireResult, imageIndex] = _swapchain->handle().acquireNextImage(UINT64_MAX, *_imageAvailable, nullptr);
        if (acquireResult != vk::Result::eSuccess)
            throw std::runtime_error("failed to acquire a swap-chain image");

        // The acquired image selects the render-finished semaphore used by
        // both queue submission and presentation for this frame.
        auto& renderFinished = _renderFinished[imageIndex];

        // Record commands for the image acquired above. This includes the
        // image layout transitions, dynamic rendering, and triangle draw.
        _commandBuffer->record(imageIndex, *_swapchain, *_pipeline);

        // Do not execute the command buffer until the acquired image is ready.
        // Waiting at color-attachment output allows earlier shader work to begin.
        const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        const vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*_imageAvailable,
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &*_commandBuffer->buffer(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderFinished};

        // Signal renderFinished when the GPU has completed the command buffer.
        // The fence is signaled at the same time for the next CPU frame.
        _device->graphicsQueue().submit(submitInfo, *_drawFence);

        // Presentation waits for renderFinished, ensuring the image is fully
        // rendered before it is handed back to the presentation engine.
        const vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinished,
            .swapchainCount = 1,
            .pSwapchains = &*_swapchain->handle(),
            .pImageIndices = &imageIndex};

        if (_device->graphicsQueue().presentKHR(presentInfo) != vk::Result::eSuccess)
            throw std::runtime_error("failed to present a swap-chain image");
    }

    std::unique_ptr<Window> _window;
    std::unique_ptr<VulkanInstance> _instance;
    std::unique_ptr<vk::raii::SurfaceKHR> _surface;
    std::unique_ptr<Device> _device;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<GraphicsPipeline> _pipeline;
    std::unique_ptr<CommandBuffer> _commandBuffer;
    vk::raii::Semaphore _imageAvailable = nullptr;
    std::vector<vk::raii::Semaphore> _renderFinished;
    vk::raii::Fence _drawFence = nullptr;
};

int main(int argc, char** argv)
{
    try
    {
        const auto executable = std::filesystem::absolute(
            argc > 0 ? std::filesystem::path(argv[0]) : std::filesystem::current_path());

        GraphicPipelineExample app;
        app.setup(executable.parent_path());
        app.run();
    }
    catch (const std::exception& exception)
    {
        LOG_ERROR(exception.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
