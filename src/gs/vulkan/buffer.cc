#include <gs/vulkan/buffer.h>
#include <gs/vulkan/context.h>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include <cstring>

Buffer::Buffer(std::shared_ptr<VkContext> context, uint32_t size)
{
    init(context, size);
}

void Buffer::init(std::shared_ptr<VkContext> context, uint32_t size)
{
    this->context = context;
    this->size = size;

    auto& device = context->device;
    auto bytes = sizeof(Vertex) * size;

    auto host_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    create_buffer(bytes, vk::BufferUsageFlagBits::eTransferSrc, host_flags, host_buffer, host_buffer_memory);
    host_memory = device->mapMemory(host_buffer_memory, 0, bytes);

    auto local_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
    create_buffer(bytes, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, local_flags, local_buffer, local_buffer_memory);
}

void Buffer::destroy()
{
    auto& device = context->device;

    device->unmapMemory(host_buffer_memory);
    device->destroyBuffer(local_buffer);
    device->destroyBuffer(host_buffer);
    device->freeMemory(local_buffer_memory);
    device->freeMemory(host_buffer_memory);
}

void Buffer::copy_vertices(const std::vector<Vertex>& vertices)
{
    auto bytes = vertices.size() * sizeof(Vertex);
    std::memcpy(host_memory, vertices.data(), bytes);
    copy_buffer(host_buffer, local_buffer, bytes);
}

void Buffer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                           vk::Buffer& buffer, vk::DeviceMemory& buffer_memory)
{
    auto& device = context->device;

    vk::BufferCreateInfo bufferInfo({}, size, usage);
    buffer = device->createBuffer(bufferInfo);

    vk::MemoryRequirements mem_requirements = device->getBufferMemoryRequirements(buffer);

    auto memory_type_index = find_memory_type(mem_requirements.memoryTypeBits, properties, context);
    vk::MemoryAllocateInfo alloc_info(mem_requirements.size, memory_type_index);

    buffer_memory = device->allocateMemory(alloc_info);
    device->bindBufferMemory(buffer, buffer_memory, 0);
}

void Buffer::copy_buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size)
{
    auto& device = context->device;
    auto& queue = context->graphics_queue;

    vk::CommandBufferAllocateInfo alloc_info(context->command_pool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command_buffer = device->allocateCommandBuffers(alloc_info)[0];

    vk::BufferCopy copy_region(0, 0, size);
    command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    command_buffer.copyBuffer(src_buffer, dst_buffer, copy_region);
    command_buffer.end();

    vk::SubmitInfo submit_info({}, {}, {}, 1, &command_buffer);
    queue.submit(submit_info, nullptr);
    queue.waitIdle();

    device->freeCommandBuffers(context->command_pool, command_buffer);
}

uint32_t Buffer::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties, std::shared_ptr<VkContext> context)
{
    vk::PhysicalDeviceMemoryProperties mem_properties = context->physical_device.getMemoryProperties();

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        auto flags = mem_properties.memoryTypes[i].propertyFlags;
        if ((type_filter & (1 << i)) && (flags & properties) == properties)
            return i;
    }

    throw std::runtime_error("[VK] Failed to find suitable memory type!");
}
