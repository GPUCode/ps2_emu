#include <gs/vulkan/texture.h>
#include <gs/vulkan/buffer.h>
#include <gs/vulkan/context.h>

VkTexture::VkTexture(std::shared_ptr<VkContext> context) :
    context(context)
{
}

VkTexture::~VkTexture()
{
}

void VkTexture::create(int width, int height, vk::ImageType type, vk::Format format)
{
    this->format = format;
    this->width = width;
    this->height = height;

    auto& device = context->device;

    int image_size = 0;
    switch (format)
    {
    case vk::Format::eR8G8B8A8Uint:
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eR32Uint:
        image_size = width * height * 4;
        break;
    case vk::Format::eR8G8B8Uint:
        image_size = width * height * 3;
        break;
    default:
        throw std::runtime_error("[VK] Unknown texture format");
    }

    create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible |
                  vk::MemoryPropertyFlagBits::eHostCoherent, staging, staging_memory);
    pixels = device->mapMemory(staging_memory, 0, image_size);

    // Create the texture
    vk::ImageCreateInfo image_info
    (
        {},
        type,
        format,
        { width, height, 1 }, 1, 1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
    );

    texture = device->createImage(image_info);

    // Create texture memory
    auto requirements = device->getImageMemoryRequirements(texture);

    auto memory_index = Buffer::find_memory_type(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, context);
    vk::MemoryAllocateInfo alloc_info(requirements.size, memory_index);

    texture_memory = device->allocateMemory(alloc_info);
    device->bindImageMemory(texture, texture_memory, 0);

    // Create texture view
    vk::ImageViewCreateInfo view_info({}, texture, vk::ImageViewType::e1D, format, {},
                                     vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    texture_view = device->createImageView(view_info);

    // Create texture sampler
    auto props = context->physical_device.getProperties();
    vk::SamplerCreateInfo sampler_info({}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge,
                                      vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {}, true, props.limits.maxSamplerAnisotropy,
                                      false, vk::CompareOp::eAlways, {}, {}, vk::BorderColor::eIntOpaqueBlack, false);

    texture_sampler = device->createSampler(sampler_info);
}

void VkTexture::destroy()
{
    auto& device = context->device;

    device->waitIdle();
    device->destroyImageView(texture_view);
    device->destroyImage(texture);
    device->freeMemory(texture_memory);
    device->unmapMemory(staging_memory);
    device->freeMemory(staging_memory);
    device->destroySampler(texture_sampler);
    device->destroyBuffer(staging);
}

void VkTexture::transition_layout(vk::ImageLayout old_layout, vk::ImageLayout new_layout)
{
    auto& device = context->device;
    auto& queue = context->graphics_queue;

    vk::CommandBufferAllocateInfo alloc_info(context->command_pool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command_buffer = device->allocateCommandBuffers(alloc_info)[0];

    command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::ImageMemoryBarrier barrier({}, {}, old_layout, new_layout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, texture,
                                   vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    std::array<vk::ImageMemoryBarrier, 1> barriers = { barrier };

    vk::PipelineStageFlags source_stage, destination_stage;
    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        source_stage = vk::PipelineStageFlagBits::eTransfer;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("[VK] Unsupported layout transition!");
    }

    command_buffer.pipelineBarrier(source_stage, destination_stage, vk::DependencyFlagBits::eByRegion, {}, {}, barriers);
    command_buffer.end();

    vk::SubmitInfo submit_info({}, {}, {}, 1, &command_buffer);
    queue.submit(submit_info, nullptr);
    queue.waitIdle();

    device->freeCommandBuffers(context->command_pool, command_buffer);
}

void VkTexture::copy_pixels(const std::vector<uint8_t>& new_pixels)
{
    auto& device = context->device;
    auto& queue = context->graphics_queue;

    // Transition image to transfer format
    transition_layout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    // Copy pixels to staging buffer
    std::memcpy(pixels, new_pixels.data(), new_pixels.size() * sizeof(new_pixels[0]));

    // Copy the staging buffer to the image
    vk::CommandBufferAllocateInfo alloc_info(context->command_pool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command_buffer = device->allocateCommandBuffers(alloc_info)[0];

    command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::BufferImageCopy region(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), {0}, {width,height,1});
    std::array<vk::BufferImageCopy, 1> regions = { region };

    command_buffer.copyBufferToImage(staging, texture, vk::ImageLayout::eTransferDstOptimal, regions);
    command_buffer.end();

    vk::SubmitInfo submit_info({}, {}, {}, 1, &command_buffer);
    queue.submit(submit_info, nullptr);
    queue.waitIdle();

    device->freeCommandBuffers(context->command_pool, command_buffer);

    // Prepare for shader reads
    transition_layout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void VkTexture::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                           vk::Buffer& buffer, vk::DeviceMemory& buffer_memory)
{
    auto& device = context->device;

    vk::BufferCreateInfo bufferInfo({}, size, usage);
    buffer = device->createBuffer(bufferInfo);

    vk::MemoryRequirements mem_requirements = device->getBufferMemoryRequirements(buffer);

    auto memory_type_index = Buffer::find_memory_type(mem_requirements.memoryTypeBits, properties, context);
    vk::MemoryAllocateInfo alloc_info(mem_requirements.size, memory_type_index);

    buffer_memory = device->allocateMemory(alloc_info);
    device->bindBufferMemory(buffer, buffer_memory, 0);
}
