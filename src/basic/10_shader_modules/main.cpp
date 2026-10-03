#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include "device.hpp"
#include "graphicsPipeline.hpp"
#include "log.h"
#include "swapchain.hpp"
#include "vulkan_config.hpp"
#include "vulkan_instance.hpp"
#include "window.hpp"

class ShaderModulesExample
{
public:
    void setup(const std::filesystem::path& directory)
    {
        _window = std::make_unique<Window>(vulkan_config::windowWidth, vulkan_config::windowHeight, vulkan_config::applicationName);
        _instance = std::make_unique<VulkanInstance>();
        _surface = std::make_unique<vk::raii::SurfaceKHR>(_instance->createSurface(*_window));
        _device = std::make_unique<Device>(*_instance, *_surface);
        _swapchain = std::make_unique<Swapchain>(*_device, *_surface, *_window);
        _pipeline = std::make_unique<GraphicsPipeline>(*_device, directory);
    }
    void run()
    {
        while (!_window->shouldClose())
            _window->pollEvents();
    }
private:
    std::unique_ptr<Window> _window;
    std::unique_ptr<VulkanInstance> _instance;
    std::unique_ptr<vk::raii::SurfaceKHR> _surface;
    std::unique_ptr<Device> _device;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<GraphicsPipeline> _pipeline;
};

int main(int argc, char** argv)
{
    try
    {
        const auto executable = std::filesystem::absolute(
            argc > 0 ? std::filesystem::path(argv[0]) : std::filesystem::current_path());

        ShaderModulesExample app;
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
