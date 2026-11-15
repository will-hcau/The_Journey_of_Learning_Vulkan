#pragma once

#include <cstddef>
#include <vulkan/vulkan_raii.hpp>

#include "device.hpp"

/** Owns a CPU-visible buffer containing the interleaved vertices supplied by the example. */
class VertexBuffer
{
public:
    VertexBuffer(const Device&, const void* data, vk::DeviceSize size);

    const vk::raii::Buffer& handle() const { return _buffer; }

private:
    const Device& _device;
    vk::raii::DeviceMemory _memory = nullptr;
    vk::raii::Buffer _buffer = nullptr;
};
