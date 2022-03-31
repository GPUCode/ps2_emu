#include <gs/vulkan/buffer.h>
#include <gs/vulkan/context.h>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include <cstring>

Buffer::Buffer(std::shared_ptr<VkContext> context) :
    context(context)
{
}

Buffer::~Buffer()
{
    auto& device = context->device;
    if (memory != nullptr)
        device->unmapMemory(buffer_memory.get());
}

void Buffer::create(uint32_t byte_count, vk::MemoryPropertyFlags properties, vk::BufferUsageFlags usage)
{
    auto& device = context->device;

    vk::BufferCreateInfo bufferInfo({}, byte_count, usage);
    buffer = device->createBufferUnique(bufferInfo);

    auto mem_requirements = device->getBufferMemoryRequirements(buffer.get());

    auto memory_type_index = find_memory_type(mem_requirements.memoryTypeBits, properties, context);
    vk::MemoryAllocateInfo alloc_info(mem_requirements.size, memory_type_index);

    buffer_memory = device->allocateMemoryUnique(alloc_info);
    device->bindBufferMemory(buffer.get(), buffer_memory.get(), 0);

    if (properties & vk::MemoryPropertyFlagBits::eHostVisible)
        memory = device->mapMemory(buffer_memory.get(), 0, byte_count);
}

void Buffer::bind(vk::CommandBuffer& command_buffer)
{
    vk::DeviceSize offsets[1] = { 0 };
    command_buffer.bindVertexBuffers(0, 1, &buffer.get(), offsets);
}

void Buffer::copy_buffer(Buffer& src_buffer, Buffer& dst_buffer, const vk::BufferCopy& region)
{
    auto& context = src_buffer.context;
    auto& device = context->device;
    auto& queue = context->graphics_queue;

    vk::CommandBufferAllocateInfo alloc_info(context->command_pool.get(), vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command_buffer = device->allocateCommandBuffers(alloc_info)[0];

    command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    command_buffer.copyBuffer(src_buffer.buffer.get(), dst_buffer.buffer.get(), region);
    command_buffer.end();

    vk::SubmitInfo submit_info({}, {}, {}, 1, &command_buffer);
    queue.submit(submit_info, nullptr);
    queue.waitIdle();

    device->freeCommandBuffers(context->command_pool.get(), command_buffer);
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

VertexBuffer::VertexBuffer(const std::shared_ptr<VkContext>& context) :
    host(context), local(context), context(context)
{
}

void VertexBuffer::create(uint32_t vertex_count)
{
    // Create a host and local buffer
    auto byte_count = sizeof(Vertex) * vertex_count;
    local.create(byte_count, vk::MemoryPropertyFlagBits::eDeviceLocal,
                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
    host.create(byte_count, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::BufferUsageFlagBits::eTransferSrc);
}

void VertexBuffer::copy_vertices(Vertex* vertices, uint32_t count)
{
    auto byte_count = count * sizeof(Vertex);
    std::memcpy(host.memory, vertices, byte_count);
    Buffer::copy_buffer(host, local, { 0, 0, byte_count });
}

void VertexBuffer::bind(vk::CommandBuffer& command_buffer)
{
    local.bind(command_buffer);
}
