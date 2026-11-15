#pragma once

#include <array>
#include <cstddef>

#include <vulkan/vulkan_raii.hpp>

/** The vertex shape shared by main.cpp and the pipeline helper. */
struct TriangleVertex
{
    float position[2];
    float color[3];

    static vk::VertexInputBindingDescription bindingDescription()
    {
        return {.binding = 0, .stride = sizeof(TriangleVertex), .inputRate = vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions()
    {
        return {{
            {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(TriangleVertex, position)},
            {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(TriangleVertex, color)}}};
    }
};
