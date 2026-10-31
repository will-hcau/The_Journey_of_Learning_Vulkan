#include <array>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include "commandBuffer.hpp"
#include "device.hpp"
#include "graphicsPipeline.hpp"
#include "log.h"
#include "swapchain.hpp"
#include "vulkan_config.hpp"
#include "vulkan_instance.hpp"
#include "window.hpp"

namespace
{
// Two frame slots allow CPU submission to overlap work already queued on the GPU.
constexpr uint32_t framesInFlight = 2;
}

class FrameInFlightExample
{
public:
    void setup(const std::filesystem::path& directory)
    {
        _window = std::make_unique<Window>(vulkan_config::windowWidth, vulkan_config::windowHeight, vulkan_config::applicationName);
        _instance = std::make_unique<VulkanInstance>();
        _surface = std::make_unique<vk::raii::SurfaceKHR>(_instance->createSurface(*_window));
        _device = std::make_unique<Device>(*_instance, *_surface);
        _swapchain = std::make_unique<Swapchain>(*_device, *_surface, *_window);
        _pipeline = std::make_unique<GraphicsPipeline>(*_device, *_swapchain, directory);
        _commandBuffers = std::make_unique<CommandBuffer>(*_device, framesInFlight);

        // Keep synchronization per CPU frame slot, as in the tutorial.
        for (uint32_t frame = 0; frame < framesInFlight; ++frame)
        {
            _imageAvailable.push_back(vk::raii::Semaphore(_device->logicalDevice(), vk::SemaphoreCreateInfo{}));
            _frameFences.push_back(vk::raii::Fence(
                _device->logicalDevice(),
                vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
        }

        // Presentation may still be waiting on a semaphore after the graphics
        // fence signals. Tie each render-finished semaphore to an image and
        // reuse it only after that image has been acquired again.
        for (std::size_t image = 0; image < _swapchain->images().size(); ++image)
            _renderFinished.emplace_back(_device->logicalDevice(), vk::SemaphoreCreateInfo{});

        // Track which frame fence last submitted work for each swap-chain image.
        _imagesInFlight.resize(_swapchain->images().size(), vk::Fence{});
    }

    void run()
    {
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
        auto& frameFence = _frameFences[_currentFrame];

        // Do not reuse this frame slot until its previous GPU work is complete.
        if (_device->logicalDevice().waitForFences(*frameFence, vk::True, UINT64_MAX) != vk::Result::eSuccess)
            throw std::runtime_error("failed to wait for frame fence");

        auto [acquireResult, imageIndex] = _swapchain->handle().acquireNextImage(
            UINT64_MAX, *_imageAvailable[_currentFrame], nullptr);
        if (acquireResult != vk::Result::eSuccess)
            throw std::runtime_error("failed to acquire a swap-chain image");

        // A swap-chain image may still be used by a different frame slot.
        // Wait for that submission before recording into and reusing its image.
        if (_imagesInFlight[imageIndex] &&
            _device->logicalDevice().waitForFences(_imagesInFlight[imageIndex], vk::True, UINT64_MAX) != vk::Result::eSuccess)
            throw std::runtime_error("failed to wait for swap-chain image fence");
        _imagesInFlight[imageIndex] = *frameFence;

        _commandBuffers->record(_currentFrame, imageIndex, *_swapchain, *_pipeline);

        // Reset only after acquisition succeeds; otherwise an early return
        // could leave the fence unsignaled and deadlock the next frame.
        _device->logicalDevice().resetFences(*frameFence);

        const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        const auto& commandBuffer = _commandBuffers->buffer(_currentFrame);
        auto& renderFinished = _renderFinished[imageIndex];
        const vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*_imageAvailable[_currentFrame],
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderFinished};

        _device->graphicsQueue().submit(submitInfo, *frameFence);

        const vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinished,
            .swapchainCount = 1,
            .pSwapchains = &*_swapchain->handle(),
            .pImageIndices = &imageIndex};

        if (_device->graphicsQueue().presentKHR(presentInfo) != vk::Result::eSuccess)
            throw std::runtime_error("failed to present a swap-chain image");

        // Advance the independent CPU frame slot, not the acquired image index.
        _currentFrame = (_currentFrame + 1) % framesInFlight;
    }

    std::unique_ptr<Window> _window;
    std::unique_ptr<VulkanInstance> _instance;
    std::unique_ptr<vk::raii::SurfaceKHR> _surface;
    std::unique_ptr<Device> _device;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<GraphicsPipeline> _pipeline;
    std::unique_ptr<CommandBuffer> _commandBuffers;

    std::vector<vk::raii::Semaphore> _imageAvailable;
    std::vector<vk::raii::Semaphore> _renderFinished;
    std::vector<vk::raii::Fence> _frameFences;
    std::vector<vk::Fence> _imagesInFlight;
    uint32_t _currentFrame = 0;
};

int main(int argc, char** argv)
{
    try
    {
        const auto executable = std::filesystem::absolute(
            argc > 0 ? std::filesystem::path(argv[0]) : std::filesystem::current_path());

        FrameInFlightExample app;
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
