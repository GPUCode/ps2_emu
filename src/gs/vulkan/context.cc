#include <gs/vulkan/context.h>
#include <gs/vulkan/buffer.h>
#include <gs/vulkan/window.h>
#include <gs/vulkan/texture.h>
#include <fstream>
#include <array>

VkContext::VkContext(vk::Instance instance_, VkWindow* window) :
    instance(instance_), window(window)
{
    // Make sure the physical devices are available on construction
    create_devices();
}

VkContext::~VkContext()
{
    device->waitIdle();

    device->destroyDescriptorSetLayout(descriptor_layout);
    device->destroyDescriptorPool(descriptor_pool);
    device->freeCommandBuffers(command_pool, command_buffers);
    device->destroyPipelineLayout(pipeline_layout);
    device->destroyPipeline(graphics_pipeline);
    device->destroyRenderPass(renderpass);
    device->destroyCommandPool(command_pool);
}

void VkContext::init()
{
    // Initialize context
    create_renderpass();
    create_command_pool();
    create_command_buffers();
}

vk::CommandBuffer& VkContext::get_command_buffer()
{
    return command_buffers[window->image_index];
}

VkShader VkContext::create_shader_module(std::filesystem::path filepath)
{
    VkShader shader(device.get());
    std::ifstream shaderfile(filepath.c_str(), std::ios::ate | std::ios::binary);

    if (!shaderfile.is_open())
        throw std::runtime_error("[UTIL] Failed to open shader file!");

    size_t size = shaderfile.tellg();
    std::vector<char> buffer(size);

    shaderfile.seekg(0);
    shaderfile.read(buffer.data(), size);
    shaderfile.close();

    shader.module = device->createShaderModule({ {}, buffer.size(), reinterpret_cast<const uint32_t*>(buffer.data()) });
    return shader;
}

void VkContext::create_devices(int device_id)
{
    // Pick a physical device
    auto physical_devices = instance->enumeratePhysicalDevices();
    physical_device = physical_devices.front();

    // Get available queue family properties
    auto family_props = physical_device.getQueueFamilyProperties();

    // Discover a queue with both graphics and compute capabilities
    vk::QueueFlags search = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
    for (int i = 0; i < family_props.size(); i++)
    {
        auto& family = family_props[i];
        if ((family.queueFlags & search) == search)
            queue_family = i;
    }

    if (queue_family == -1)
        throw std::runtime_error("[VK] Could not find appropriate queue families!\n");

    const float default_queue_priority = 0.0f;
    std::array<const char*, 1> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    auto queue_info = vk::DeviceQueueCreateInfo({}, queue_family, 1, &default_queue_priority);

    std::array<vk::PhysicalDeviceFeatures, 1> features = {};
    features[0].samplerAnisotropy = true;

    vk::DeviceCreateInfo device_info({}, 1, &queue_info, 0, nullptr, device_extensions.size(), device_extensions.data(), features.data());
    device = physical_device.createDeviceUnique(device_info);

    graphics_queue = device->getQueue(queue_family, 0);
}

void VkContext::create_renderpass()
{
    // Color attachment
    vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depth_attachment_ref(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::AttachmentDescription attachments[2] =
    {
        {
            {},
            window->swapchain_info.surface_format.format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        },
        {
            {},
            window->swapchain_info.depth_format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        }
    };

    vk::SubpassDependency dependency
    (
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        vk::AccessFlagBits::eNone,
        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::DependencyFlagBits::eByRegion
    );

    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, {}, 1, &color_attachment_ref, {}, &depth_attachment_ref);
    vk::RenderPassCreateInfo renderpass_info({}, 2, attachments, 1, &subpass, 1, &dependency);
    renderpass = device->createRenderPass(renderpass_info);
}

vk::UniquePipelineLayout VkContext::create_pipeline_layout(const PipelineLayoutInfo& info) const
{
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo
    (
        {},
        info.set_layouts.size(),
        info.set_layouts.data(),
        info.push_const_ranges.size(),
        info.push_const_ranges.data()
    );

    return device->createPipelineLayoutUnique(pipelineLayoutInfo);
}

void VkContext::create_graphics_pipeline(VkShader& vertex, VkShader& fragment)
{
    vk::PipelineShaderStageCreateInfo shader_stages[] =
    {
        {
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertex.module,
            "main"
        },
        {
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragment.module,
            "main"
        }
    };

    vk::PipelineVertexInputStateCreateInfo vertex_input_info
    (
        {},
        1,
        &Vertex::binding_desc,
        Vertex::attribute_desc.size(),
        Vertex::attribute_desc.data()
    );

    vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
    vk::Viewport viewport(0, 0, window->swapchain_info.extent.width, window->swapchain_info.extent.height, 0, 1);
    vk::Rect2D scissor({ 0, 0 }, window->swapchain_info.extent);

    vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1, &scissor);
    vk::PipelineRasterizationStateCreateInfo rasterizer
    (
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eClockwise,
        VK_FALSE
    );

    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, VK_FALSE);
    vk::PipelineColorBlendAttachmentState colorblend_attachment(VK_FALSE);
    colorblend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo color_blending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &colorblend_attachment, {0});
    vk::PipelineLayoutCreateInfo pipeline_layout_info({}, 1, &descriptor_layout, 0, nullptr);
    try {
        pipeline_layout = device->createPipelineLayout(pipeline_layout_info);
    } catch (vk::SystemError err) {
        throw std::runtime_error("[VK] Failed to create pipeline layout!");
    }

    vk::DynamicState dynamic_states[2] = { vk::DynamicState::eDepthCompareOp, vk::DynamicState::eLineWidth };
    vk::PipelineDynamicStateCreateInfo dynamic_info({}, 2, dynamic_states);

    // Depth and stencil state containing depth and stencil compare and test operations
    // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
    vk::PipelineDepthStencilStateCreateInfo depth_info({}, VK_TRUE, VK_TRUE, vk::CompareOp::eGreaterOrEqual, VK_FALSE, VK_TRUE);
    depth_info.back.failOp = vk::StencilOp::eKeep;
    depth_info.back.passOp = vk::StencilOp::eKeep;
    depth_info.back.compareOp = vk::CompareOp::eAlways;
    depth_info.front = depth_info.back;

    vk::GraphicsPipelineCreateInfo pipeline_info
    (
        {},
        2,
        shader_stages,
        &vertex_input_info,
        &input_assembly,
        nullptr,
        &viewport_state,&rasterizer,
        &multisampling,
        &depth_info,
        &color_blending,
        &dynamic_info,
        pipeline_layout,
        renderpass
    );

    auto pipeline = device->createGraphicsPipeline(nullptr, pipeline_info);
    if (pipeline.result == vk::Result::eSuccess)
        graphics_pipeline = pipeline.value;
    else
        throw std::runtime_error("[VK] Couldn't create graphics pipeline");
}

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

void VkContext::create_descriptor_sets(VkTexture& texture)
{
    vk::DescriptorSetLayoutBinding sampler_layout_binding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

    std::array<vk::DescriptorSetLayoutBinding, 1> bindings = { sampler_layout_binding };
    vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings.size(), bindings.data());

    descriptor_layout = device->createDescriptorSetLayout(layout_info);

    std::array<vk::DescriptorPoolSize, 1> pool_sizes = { vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT) };
    vk::DescriptorPoolCreateInfo pool_info({}, MAX_FRAMES_IN_FLIGHT, pool_sizes.size(), pool_sizes.data());

    descriptor_pool = device->createDescriptorPool(pool_info);

    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_layout);
    vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, MAX_FRAMES_IN_FLIGHT, layouts.data());

    descriptor_sets = device->allocateDescriptorSets(alloc_info);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        auto& set = descriptor_sets[i];

        vk::DescriptorImageInfo image_info(texture.texture_sampler, texture.texture_view, vk::ImageLayout::eShaderReadOnlyOptimal);
        std::array<vk::WriteDescriptorSet, 1> descriptor_writes =
        {
            vk::WriteDescriptorSet(set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info)
        };

        device->updateDescriptorSets(descriptor_writes, {});
   }
}

void VkContext::create_command_pool()
{
    vk::CommandPoolCreateInfo pool_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family);
    command_pool = device->createCommandPool(pool_info);
}

void VkContext::create_command_buffers()
{
    command_buffers.resize(window->swapchain_info.image_count);

    vk::CommandBufferAllocateInfo alloc_info(command_pool, vk::CommandBufferLevel::ePrimary, command_buffers.size());
    command_buffers = device->allocateCommandBuffers(alloc_info);
}

void VkContext::destroy()
{
    device->waitIdle();
}
