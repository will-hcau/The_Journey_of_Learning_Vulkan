#include "graphicsPipeline.hpp"

#include <fstream>
#include <stdexcept>

#include "log.h"

GraphicsPipeline::GraphicsPipeline(const Device& device, const std::filesystem::path& executableDirectory)
    : _device(device)
{
    const auto shaderCode = readShader(executableDirectory / "slang.spv");

    _shaderModule = createShaderModule(_device, shaderCode);

    const vk::PipelineShaderStageCreateInfo vertexStage{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *_shaderModule,
        .pName = "vertMain"};

    const vk::PipelineShaderStageCreateInfo fragmentStage{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *_shaderModule,
        .pName = "fragMain"};

    (void)vertexStage;
    (void)fragmentStage;

    LOG_INFO("Shader module loaded from " << (executableDirectory / "slang.spv").string());
}

std::vector<char> GraphicsPipeline::readShader(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file)
        throw std::runtime_error("failed to open shader binary: " + path.string());

    const auto size = file.tellg();
    std::vector<char> code(static_cast<size_t>(size));

    file.seekg(0, std::ios::beg);
    file.read(code.data(), size);

    if (!file)
        throw std::runtime_error("failed to read shader binary: " + path.string());

    return code;
}

vk::raii::ShaderModule GraphicsPipeline::createShaderModule(
    const Device& device,
    const std::vector<char>& code)
{
    const vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};

    return vk::raii::ShaderModule(device.logicalDevice(), createInfo);
}
