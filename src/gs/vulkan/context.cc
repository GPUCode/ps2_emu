#include <gs/vulkan/context.h>
#include <gs/vulkan/buffer.h>
#include <gs/vulkan/window.h>
#include <gs/vulkan/texture.h>
#include <fstream>
#include <array>

PipelineLayoutInfo::PipelineLayoutInfo(const std::shared_ptr<VkContext>& context) :
    context(context)
{
}

PipelineLayoutInfo::~PipelineLayoutInfo()
{
    for (int i = 0; i < shader_stages.size(); i++)
        context->device->destroyShaderModule(shader_stages[i].module);
}

void PipelineLayoutInfo::add_shader_module(std::string_view filepath, vk::ShaderStageFlagBits stage)
{
    std::ifstream shaderfile(filepath.data(), std::ios::ate | std::ios::binary);

    if (!shaderfile.is_open())
        throw std::runtime_error("[UTIL] Failed to open shader file!");

    size_t size = shaderfile.tellg();
    std::vector<char> buffer(size);

    shaderfile.seekg(0);
    shaderfile.read(buffer.data(), size);
    shaderfile.close();

    auto module = context->device->createShaderModule({ {}, buffer.size(), reinterpret_cast<const uint32_t*>(buffer.data()) });
    shader_stages.emplace_back(vk::PipelineShaderStageCreateFlags(), stage, module, "main");
}

void PipelineLayoutInfo::add_resource(Resource* resource, vk::DescriptorType type, vk::ShaderStageFlags stages, int binding, int group)
{
    resource_types[group].first[binding] = resource;
    resource_types[group].second.emplace_back(binding, type, 1, stages);
    needed[type]++;
}

VkContext::VkContext(vk::UniqueInstance&& instance_, VkWindow* window) :
    instance(std::move(instance_)), window(window)
{
    create_devices();
}

VkContext::~VkContext()
{
    device->waitIdle();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        for (int j = 0; j < descriptor_sets.size(); j++)
            device->destroyDescriptorSetLayout(descriptor_layouts[i][j]);
}

void VkContext::create(SwapchainInfo& info)
{
    swapchain_info = info;

    // Initialize context
    create_renderpass();
    create_command_buffers();
}

vk::CommandBuffer& VkContext::get_command_buffer()
{
    return command_buffers[window->image_index].get();
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
    for (size_t i = 0; i < family_props.size(); i++)
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
    renderpass = device->createRenderPassUnique(renderpass_info);
}

void VkContext::create_decriptor_sets(PipelineLayoutInfo &info)
{
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    pool_sizes.reserve(info.needed.size());
    for (const auto& [type, count] : info.needed)
    {
        pool_sizes.emplace_back(type, count * MAX_FRAMES_IN_FLIGHT);
    }

    for (const auto& [group, resource_info] : info.resource_types)
    {
        auto& bindings = resource_info.second;
        vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings.size(), bindings.data());

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            descriptor_layouts[i].push_back(device->createDescriptorSetLayout(layout_info));
    }

    vk::DescriptorPoolCreateInfo pool_info({}, MAX_FRAMES_IN_FLIGHT * descriptor_layouts[0].size(), pool_sizes.size(), pool_sizes.data());
    descriptor_pool = device->createDescriptorPoolUnique(pool_info);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool.get(), descriptor_layouts[i]);
        descriptor_sets[i] = device->allocateDescriptorSets(alloc_info);

        for (const auto& [group, resource_info] : info.resource_types)
        {
            auto& bindings = resource_info.second;
            std::array<vk::DescriptorImageInfo, MAX_BINDING_COUNT> image_infos;
            std::array<vk::DescriptorBufferInfo, MAX_BINDING_COUNT> buffer_infos;

            std::vector<vk::WriteDescriptorSet> descriptor_writes;
            descriptor_writes.reserve(bindings.size());

            auto& set = descriptor_sets[i][group];
            for (int j = 0; j < bindings.size(); j++)
            {
                switch (bindings[j].descriptorType)
                {
                case vk::DescriptorType::eCombinedImageSampler:
                {
                    VkTexture* texture = reinterpret_cast<VkTexture*>(resource_info.first[j]);
                    image_infos[j] = vk::DescriptorImageInfo(texture->texture_sampler.get(), texture->texture_view.get(),
                                                             vk::ImageLayout::eShaderReadOnlyOptimal);
                    descriptor_writes.emplace_back(set, j, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_infos[j]);
                    break;
                }
                case vk::DescriptorType::eStorageTexelBuffer:
                {
                    Buffer* buffer = reinterpret_cast<Buffer*>(resource_info.first[j]);
                    descriptor_writes.emplace_back(set, j, 0, 1, vk::DescriptorType::eStorageTexelBuffer, nullptr, nullptr, &buffer->buffer_view.get());
                    break;
                }
                default:
                    throw std::runtime_error("[VK] Unknown resource");
                }
            }

            device->updateDescriptorSets(descriptor_writes, {});
            descriptor_writes.clear();
        }
    }
}

void VkContext::create_graphics_pipeline(PipelineLayoutInfo& info)
{
    create_decriptor_sets(info);

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

    vk::PipelineLayoutCreateInfo pipeline_layout_info({}, descriptor_layouts[0], {});
    pipeline_layout = device->createPipelineLayoutUnique(pipeline_layout_info);

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
        info.shader_stages.size(),
        info.shader_stages.data(),
        &vertex_input_info,
        &input_assembly,
        nullptr,
        &viewport_state,&rasterizer,
        &multisampling,
        &depth_info,
        &color_blending,
        &dynamic_info,
        pipeline_layout.get(),
        renderpass.get()
    );

    auto pipeline = device->createGraphicsPipelineUnique(nullptr, pipeline_info);
    if (pipeline.result == vk::Result::eSuccess)
        graphics_pipeline = std::move(pipeline.value);
    else
        throw std::runtime_error("[VK] Couldn't create graphics pipeline");
}

void VkContext::create_command_buffers()
{
    vk::CommandPoolCreateInfo pool_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family);
    command_pool = device->createCommandPoolUnique(pool_info);

    command_buffers.resize(window->swapchain_info.image_count);

    vk::CommandBufferAllocateInfo alloc_info(command_pool.get(), vk::CommandBufferLevel::ePrimary, command_buffers.size());
    command_buffers = device->allocateCommandBuffersUnique(alloc_info);
}
