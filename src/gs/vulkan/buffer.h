#pragma once
#include <gs/vulkan/common.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class VkContext;

struct VertexInfo
{
    VertexInfo() = default;
    VertexInfo(glm::vec3 position, glm::vec3 color, glm::vec2 coords) :
        position(position), color(color), texcoords(coords) {};

    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texcoords;
};

struct Vertex : public VertexInfo
{
    Vertex() = default;
    Vertex(glm::vec3 position, glm::vec3 color = {}, glm::vec2 coords = {}) : VertexInfo(position, color, coords) {};
    static constexpr auto binding_desc = vk::VertexInputBindingDescription(0, sizeof(VertexInfo));
    static constexpr std::array<vk::VertexInputAttributeDescription, 3> attribute_desc =
    {
          vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexInfo, position)),
          vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexInfo, color)),
          vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(VertexInfo, texcoords)),
    };
};

class Buffer : public NonCopyable, public Resource
{
    friend class VertexBuffer;
public:
    Buffer(std::shared_ptr<VkContext> context);
    ~Buffer();

    void create(uint32_t size, vk::MemoryPropertyFlags properties, vk::BufferUsageFlags usage);
    void bind(vk::CommandBuffer& command_buffer);

    static uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties, std::shared_ptr<VkContext> context);
    static void copy_buffer(Buffer& src_buffer, Buffer& dst_buffer, const vk::BufferCopy& region);

public:
    void* memory = nullptr;
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory buffer_memory;
    vk::UniqueBufferView buffer_view;
    uint32_t size = 0;

protected:
    std::shared_ptr<VkContext> context;
};

class VertexBuffer
{
public:
    VertexBuffer(const std::shared_ptr<VkContext>& context);
    ~VertexBuffer() = default;

    void create(uint32_t vertex_count);
    void copy_vertices(Vertex* vertices, uint32_t count);
    void bind(vk::CommandBuffer& command_buffer);

private:
    Buffer host, local;
    std::shared_ptr<VkContext> context;
};
