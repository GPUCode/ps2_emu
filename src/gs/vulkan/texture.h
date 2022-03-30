#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class VkContext;

class VkTexture
{
public:
    VkTexture(std::shared_ptr<VkContext> context);
    ~VkTexture();

    void create(int width, int height, vk::ImageType type, vk::Format format = vk::Format::eR8G8B8A8Uint);
    void copy_pixels(const std::vector<uint8_t>& pixels);
    void destroy();

private:
    void transition_layout(vk::ImageLayout old_layout, vk::ImageLayout new_layout);
    void create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                       vk::Buffer& buffer, vk::DeviceMemory& buffer_memory);

public:
    vk::Buffer staging;
    vk::DeviceMemory staging_memory;
    uint32_t width, height;
    void* pixels;

    // Texture objects
    vk::Image texture;
    vk::ImageView texture_view;
    vk::DeviceMemory texture_memory;
    vk::Sampler texture_sampler;
    vk::Format format;

    std::shared_ptr<VkContext> context;
};
