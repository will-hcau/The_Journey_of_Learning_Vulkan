#include "graphicsPipeline.hpp"
#include <fstream>
#include <stdexcept>
#include "log.h"

GraphicsPipeline::GraphicsPipeline(
    const Device& device,
    const Swapchain& swapchain,
    const std::filesystem::path& directory)
    : _device(device)
{
    // The vertex and fragment entry points are stored in the same shader module.
    const auto code = readShader(directory / "slang.spv");
    _shaderModule = createShaderModule(_device, code);

    const vk::PipelineShaderStageCreateInfo stages[] = {
        {.stage = vk::ShaderStageFlagBits::eVertex, .module = *_shaderModule, .pName = "vertMain"},
        {.stage = vk::ShaderStageFlagBits::eFragment, .module = *_shaderModule, .pName = "fragMain"}};

    // The current shader generates its vertices, so there is no vertex buffer yet.
    const vk::PipelineVertexInputStateCreateInfo vertexInput{};

    // Interpret every group of three generated vertices as one triangle.
    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};

    // Viewport and scissor values are supplied when commands are recorded.
    const std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};
    const vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};
    const vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

    // Use filled polygons, back-face culling, and clockwise front faces.
    const vk::PipelineRasterizationStateCreateInfo rasterizer{
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .lineWidth = 1.0f};

    // Multisampling and depth/stencil testing are not used in this sample.
    const vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1};

    // Write all color channels directly without blending.
    const vk::PipelineColorBlendAttachmentState blendAttachment{
        .colorWriteMask = vk::ColorComponentFlagBits::eR |
                          vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB |
                          vk::ColorComponentFlagBits::eA};
    const vk::PipelineColorBlendStateCreateInfo colorBlending{.attachmentCount = 1, .pAttachments = &blendAttachment};

    // No descriptors or push constants are needed by the current shaders.
    _pipelineLayout = vk::raii::PipelineLayout(_device.logicalDevice(), vk::PipelineLayoutCreateInfo{});

    // Dynamic rendering supplies the swapchain color format through the pNext chain.
    const auto swapchainFormat = swapchain.imageFormat();
    const vk::PipelineRenderingCreateInfo rendering{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchainFormat};

    // This structure connects shader stages, fixed-function state, layout, and rendering format.
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> info{
        {.stageCount = 2,
         .pStages = stages,
         .pVertexInputState = &vertexInput,
         .pInputAssemblyState = &inputAssembly,
         .pViewportState = &viewportState,
         .pRasterizationState = &rasterizer,
         .pMultisampleState = &multisampling,
         .pColorBlendState = &colorBlending,
         .pDynamicState = &dynamicState,
         .layout = *_pipelineLayout},
        rendering};
    _pipeline = vk::raii::Pipeline(
        _device.logicalDevice(), nullptr, info.get<vk::GraphicsPipelineCreateInfo>());

    LOG_INFO("Graphics pipeline created.");
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

vk::raii::ShaderModule GraphicsPipeline::createShaderModule(const Device& device, const std::vector<char>& code)
{
    const vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};

    return vk::raii::ShaderModule(device.logicalDevice(), createInfo);
}
