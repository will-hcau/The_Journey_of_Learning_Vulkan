#pragma once

#include <filesystem>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "device.hpp"
#include "swapchain.hpp"

/** Owns the shader module, pipeline layout, and graphics pipeline. */
class GraphicsPipeline
{
public:
    GraphicsPipeline(const Device&, const Swapchain&, const std::filesystem::path&);
    const vk::raii::Pipeline& pipeline() const { return _pipeline; }

private:
    static std::vector<char> readShader(const std::filesystem::path&);
    static vk::raii::ShaderModule createShaderModule(const Device&, const std::vector<char>&);

    const Device& _device;
    vk::raii::ShaderModule _shaderModule = nullptr;
    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline _pipeline = nullptr;
};
