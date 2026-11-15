#include "vertex.hpp"

#include <cstring>
#include <stdexcept>

VertexBuffer::VertexBuffer(const Device& device, const void* data, vk::DeviceSize size)
    : _device(device)
{
    // Allocate a vertex buffer in host-visible memory so CPU data can be copied directly.
    const vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive};
    _buffer = vk::raii::Buffer(_device.logicalDevice(), bufferInfo);

    const auto requirements = _buffer.getMemoryRequirements();
    const auto memoryProperties = _device.physicalDevice().getMemoryProperties();
    uint32_t memoryType = UINT32_MAX;

    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
    {
        const bool allowed = (requirements.memoryTypeBits & (1u << index)) != 0;
        const auto flags = memoryProperties.memoryTypes[index].propertyFlags;
        const bool hostVisible = (flags & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{};
        const bool coherent = (flags & vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlags{};
        if (allowed && hostVisible && coherent)
        {
            memoryType = index;
            break;
        }
    }

    if (memoryType == UINT32_MAX)
        throw std::runtime_error("no host-visible coherent memory type for vertex buffer");

    _memory = vk::raii::DeviceMemory(
        _device.logicalDevice(), vk::MemoryAllocateInfo{.allocationSize = requirements.size, .memoryTypeIndex = memoryType});
    _buffer.bindMemory(*_memory, 0);

    // Host-coherent memory makes the copied vertex values visible to the GPU without a flush.
    void* mapped = _memory.mapMemory(0, size);
    std::memcpy(mapped, data, static_cast<std::size_t>(size));
    _memory.unmapMemory();
}
