#pragma once
#include <vulkan/vulkan.hpp>
#include <filesystem>
#include <optional>

class VkWindow;

// Vulkan shader abstraction
struct VkShader
{
    VkShader(vk::Device& device) : device(device) {};
    VkShader(vk::ShaderStageFlagBits stage, vk::ShaderModule module, vk::Device& device) :
        module(module), stage(stage), device(device) {}
    ~VkShader()
    {
        device.destroyShaderModule(module);
    }

    // Make object non-copyable
    VkShader(const VkShader&) = delete;
    VkShader& operator=(const VkShader&) = delete;
    VkShader(VkShader&&) = default;

    vk::ShaderModule module;
    vk::ShaderStageFlagBits stage;
    vk::Device& device;
};

struct PipelineLayoutInfo
{
    std::vector<vk::DescriptorSetLayout> set_layouts;
    std::vector<vk::PushConstantRange> push_const_ranges;
};

class VkTexture;

// The vulkan context. Can only be created by the window
class VkContext
{
    friend class VkWindow;
public:
    VkContext(vk::Instance instance, VkWindow* window);
    ~VkContext();
    VkContext(VkContext&&) = default;

    // Shader creation helpers
    VkShader create_shader_module(std::filesystem::path filepath);
    vk::UniquePipelineLayout create_pipeline_layout(const PipelineLayoutInfo& info) const;
    void create_graphics_pipeline(VkShader& vertex, VkShader& fragment);
    void create_descriptor_sets(VkTexture& texture);

    void init();
    void destroy();

    vk::CommandBuffer& get_command_buffer();

private:
    void create_devices(int device_id = 0);
    void create_renderpass();
    void create_command_pool();
    void create_command_buffers();

public:
    // Queue family indexes
    uint32_t queue_family = -1;

    // Core vulkan objects
    vk::UniqueInstance instance;
    vk::PhysicalDevice physical_device;
    vk::UniqueDevice device;
    vk::Queue graphics_queue;

    // Pipeline
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline graphics_pipeline;
    vk::RenderPass renderpass;
    vk::DescriptorSetLayout descriptor_layout;
    vk::DescriptorPool descriptor_pool;
    std::vector<vk::DescriptorSet> descriptor_sets;

    // Command buffer
    vk::CommandPool command_pool;
    std::vector<vk::CommandBuffer> command_buffers;

    // Window
    VkWindow* window;
};
