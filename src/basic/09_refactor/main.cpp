#include <cstdlib>
#include <memory>
#include <stdexcept>

#include "device.hpp"
#include "log.h"
#include "swapchain.hpp"
#include "vulkan_config.hpp"
#include "vulkan_instance.hpp"
#include "window.hpp"

class SimpleTriangle
{
public:
    void setup()
    {
        _window = std::make_unique<Window>(
            vulkan_config::windowWidth,
            vulkan_config::windowHeight,
            vulkan_config::applicationName);

        _instance = std::make_unique<VulkanInstance>();
        _surface = std::make_unique<vk::raii::SurfaceKHR>(_instance->createSurface(*_window));

        _device = std::make_unique<Device>(*_instance, *_surface);
        _swapchain = std::make_unique<Swapchain>(*_device, *_surface, *_window);
    }

    void run()
    {
        // The render loop currently only processes window events.
        // Rendering commands will be added in later chapters.
        while (!_window->shouldClose())
            _window->pollEvents();
    }

private:
    std::unique_ptr<Window> _window;
    std::unique_ptr<VulkanInstance> _instance;
    std::unique_ptr<vk::raii::SurfaceKHR> _surface;
    std::unique_ptr<Device> _device;
    std::unique_ptr<Swapchain> _swapchain;
};

int main()
{
    try
    {
        SimpleTriangle app;

        app.setup();
        app.run();
    }
    catch (const std::exception& exception)
    {
        LOG_ERROR(exception.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
