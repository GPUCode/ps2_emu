#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class VkContext;

struct VertexInfo
{
    VertexInfo() = default;
    VertexInfo(glm::vec3 position, glm::vec3 color) : position(position), color(color) {};

    glm::vec3 position;
    glm::vec3 color;
};

struct Vertex : public VertexInfo
{
    Vertex() = default;
    Vertex(glm::vec3 position, glm::vec3 color = {}) : VertexInfo(position, color) {};
    static constexpr auto binding_desc = vk::VertexInputBindingDescription(0, sizeof(VertexInfo));
    static constexpr std::array<vk::VertexInputAttributeDescription, 2> attribute_desc =
    {
          vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexInfo, position)),
          vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexInfo, color)),
    };
};

class Buffer
{
public:
    Buffer() = default;
    Buffer(std::shared_ptr<VkContext> context, uint32_t size);
    ~Buffer() = default;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    static uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties, std::shared_ptr<VkContext> context);
    void init(std::shared_ptr<VkContext> context, uint32_t size);
    void copy_vertices(const std::vector<Vertex>& vertices);
    void destroy();

private:
    void copy_buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size);
    void create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                       vk::Buffer& buffer, vk::DeviceMemory& buffer_memory);

public:
    void* host_memory = nullptr;
    vk::Buffer local_buffer, host_buffer;
    vk::DeviceMemory local_buffer_memory, host_buffer_memory;
    uint32_t size = 0;

private:
    std::shared_ptr<VkContext> context;
};
