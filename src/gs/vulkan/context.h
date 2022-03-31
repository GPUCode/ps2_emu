#pragma once
#include <gs/vulkan/common.h>
#include <gs/vulkan/window.h>
#include <string_view>
#include <robin_hood.h>

class VkWindow;
class VkContext;

constexpr int MAX_BINDING_COUNT = 10;

struct PipelineLayoutInfo
{
    friend class VkContext;
    PipelineLayoutInfo(const std::shared_ptr<VkContext>& context);
    ~PipelineLayoutInfo();

    void add_shader_module(std::string_view filepath, vk::ShaderStageFlagBits stage);
    void add_resource(Resource* resource, vk::DescriptorType type, vk::ShaderStageFlags stages, int binding, int group = 0);

private:
    using DescInfo = std::pair<std::array<Resource*, MAX_BINDING_COUNT>, std::vector<vk::DescriptorSetLayoutBinding>>;

    std::shared_ptr<VkContext> context;
    robin_hood::unordered_flat_map<int, DescInfo> resource_types;
    robin_hood::unordered_flat_map<vk::DescriptorType, int> needed;
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
};

class VkTexture;

// The vulkan context. Can only be created by the window
class VkContext
{
    friend class VkWindow;
public:
    VkContext(vk::UniqueInstance&& instance, VkWindow* window);
    ~VkContext();

    void create(SwapchainInfo& info);
    void create_graphics_pipeline(PipelineLayoutInfo& info);

    vk::CommandBuffer& get_command_buffer();

private:
    void create_devices(int device_id = 0);
    void create_renderpass();
    void create_command_buffers();
    void create_decriptor_sets(PipelineLayoutInfo& info);

public:
    // Queue family indexes
    uint32_t queue_family = -1;

    // Core vulkan objects
    vk::UniqueInstance instance;
    vk::PhysicalDevice physical_device;
    vk::UniqueDevice device;
    vk::Queue graphics_queue;

    // Pipeline
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniquePipeline graphics_pipeline;
    vk::UniqueRenderPass renderpass;
    vk::UniqueDescriptorPool descriptor_pool;
    std::array<std::vector<vk::DescriptorSetLayout>, MAX_FRAMES_IN_FLIGHT> descriptor_layouts;
    std::array<std::vector<vk::DescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptor_sets;

    // Command buffer
    vk::UniqueCommandPool command_pool;
    std::vector<vk::UniqueCommandBuffer> command_buffers;

    // Window
    VkWindow* window;
    SwapchainInfo swapchain_info;
};
