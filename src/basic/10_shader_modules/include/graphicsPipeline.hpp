#pragma once

#include <filesystem>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "device.hpp"

/** @brief Loads the compiled Slang module and creates Vulkan shader modules. */
class GraphicsPipeline
{
public:
    GraphicsPipeline(const Device&, const std::filesystem::path& executableDirectory);

private:
    static std::vector<char> readShader(const std::filesystem::path&);
    static vk::raii::ShaderModule createShaderModule(const Device&, const std::vector<char>&);

    const Device& _device;
    vk::raii::ShaderModule _shaderModule = nullptr;
};
