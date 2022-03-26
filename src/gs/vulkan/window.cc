#include <gs/vulkan/window.h>
#include <gs/vulkan/context.h>
#include <gs/vulkan/buffer.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fmt/core.h>

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

VkWindow::VkWindow(int width, int height, std::string_view name) :
    width(width), height(height), name(name)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GL_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, name.data(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
    {
        auto my_window = reinterpret_cast<VkWindow*>(glfwGetWindowUserPointer(window));
        my_window->framebuffer_resized = true;
        my_window->width = width;
        my_window->height = height;
    });
}

VkWindow::~VkWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VkWindow::destroy()
{
    auto& device = context->device;
    device->waitIdle();

    buffers.clear();
    device->destroySwapchainKHR(swapchain);

    // Destroy sync objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        device->destroySemaphore(render_semaphores[i]);
        device->destroySemaphore(image_semaphores[i]);
        device->destroyFence(flight_fences[i]);
    }

    // Destroy depth buffer
    device->destroyImage(depth_buffer.image);
    device->destroyImageView(depth_buffer.view);
    device->freeMemory(depth_buffer.memory);

    context->instance->destroySurfaceKHR(surface);
}

bool VkWindow::should_close() const
{
    return glfwWindowShouldClose(window);
}

vk::Extent2D VkWindow::get_extent() const
{
    return { width, height };
}

void VkWindow::begin_frame()
{
    // Poll for mouse events
    glfwPollEvents();

    auto& device = context->device;
    auto result = device->waitForFences(1, &flight_fences[current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("[VK] Failed waiting for flight fences");
    result = device->resetFences(1, &flight_fences[current_frame]);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("[VK] Failed reseting flight fences");

    try
    {
        vk::ResultValue result = device->acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(),
            image_semaphores[current_frame], nullptr);
        image_index = result.value;
    }
    catch (vk::OutOfDateKHRError err)
    {
        //recreateSwapChain();
        return;
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Start command buffer recording
    auto& command_buffer = context->get_command_buffer();
    command_buffer.begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});

    // Clear the screen
    vk::ClearValue clear_values[2];
    clear_values[0].color = {std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clear_values[1].depthStencil = vk::ClearDepthStencilValue(0.0f, 0.0f);

    vk::Rect2D render_area({0, 0}, swapchain_info.extent);
    vk::RenderPassBeginInfo renderpass_info(context->renderpass, buffers[current_frame].framebuffer, render_area, 2, clear_values);

    command_buffer.beginRenderPass(renderpass_info, vk::SubpassContents::eInline);
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, context->graphics_pipeline);
}

std::shared_ptr<VkContext> VkWindow::create_context(bool validation)
{
    vk::ApplicationInfo app_info("PS2 Emulator", 1, nullptr, 0, VK_API_VERSION_1_3);

    uint32_t extension_count = 0U;
    const char** extension_list = glfwGetRequiredInstanceExtensions(&extension_count);

    // Get required extensions
    std::vector<const char*> extensions(extension_list, extension_list + extension_count);
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    const char* layers[1] = { "VK_LAYER_KHRONOS_validation" };
    vk::InstanceCreateInfo instance_info({}, &app_info, {}, {}, extensions.size(), extensions.data());
    if (validation)
    {
        instance_info.enabledLayerCount = 1;
        instance_info.ppEnabledLayerNames = layers;
    }

    auto instance = vk::createInstance(instance_info);

    // Create a surface for our window
    VkSurfaceKHR surface_tmp;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface_tmp) != VK_SUCCESS)
        throw std::runtime_error("[WINDOW] Could not create window surface\n");

    surface = surface_tmp;

    // Setup debug callback to display validation layer messages
    if (validation)
    {
        auto debug_callback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) -> VkBool32
        {
            fmt::print("[DEBUG] {}\n", pCallbackData->pMessage);
            return VK_FALSE;
        };

        auto dispatch = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
        auto debug_info = vk::DebugUtilsMessengerCreateInfoEXT({},
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
                          vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                          debug_callback);
        auto messenger = instance.createDebugUtilsMessengerEXTUnique(debug_info, nullptr, dispatch);
    }

    // Create context
    context = std::make_shared<VkContext>(instance, this);
    swapchain_info = get_swapchain_info();
    context->init();

    // Create swapchain
    create_present_queue();
    create_depth_buffer();
    create_swapchain();
    create_sync_objects();

    return context;
}

void VkWindow::create_present_queue()
{
    auto& physical_device = context->physical_device;
    auto family_props = physical_device.getQueueFamilyProperties();

    // Determine a queueFamilyIndex that suports present
    // first check if the graphicsQueueFamiliyIndex is good enough
    size_t present_queue_family = -1;
    if (physical_device.getSurfaceSupportKHR(context->queue_family, surface))
    {
        present_queue_family = context->queue_family;
    }
    else
    {
        // The graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both
        // graphics and present
        vk::QueueFlags search = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
        for (size_t i = 0; i < family_props.size(); i++ )
        {
            if (((family_props[i].queueFlags & search) == search) && physical_device.getSurfaceSupportKHR(i, surface))
            {
                context->queue_family = present_queue_family = i;
                break;
            }
        }

        if (present_queue_family == -1)
        {
            // There's nothing like a single family index that supports both graphics and present -> look for an other
            // family index that supports present
            for (size_t i = 0; i < family_props.size(); i++ )
            {
                if (physical_device.getSurfaceSupportKHR(i, surface ))
                {
                    present_queue_family = i;
                    break;
                }
            }
        }
    }

    if (present_queue_family == -1)
        throw std::runtime_error("[VK] No present queue could be found");

    // Get the queue
    present_queue = context->device->getQueue(present_queue_family, 0);
}

void VkWindow::create_swapchain(bool enable_vsync)
{
    auto& physical_device = context->physical_device;
    vk::SwapchainKHR old_swapchain = swapchain;

    // Figure out best swapchain create attributes
    auto capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);

    // Find the transformation of the surface, prefer a non-rotated transform
    auto pretransform = capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ?
                        vk::SurfaceTransformFlagBitsKHR::eIdentity :
                        capabilities.currentTransform;

    // Create the swapchain
    vk::SwapchainCreateInfoKHR swapchain_create_info
    (
        {},
        surface,
        swapchain_info.image_count,
        swapchain_info.surface_format.format,
        swapchain_info.surface_format.colorSpace,
        swapchain_info.extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        pretransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        swapchain_info.present_mode,
        VK_TRUE,
        old_swapchain
    );

    auto& device = context->device;
    swapchain = device->createSwapchainKHR(swapchain_create_info);

    // If an existing sawp chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if (old_swapchain)
    {
        buffers.clear();
        device->destroySwapchainKHR(old_swapchain);
    }

    // Get the swap chain images
    auto images = device->getSwapchainImagesKHR(swapchain);

    // Create the swapchain buffers containing the image and imageview
    buffers.resize(images.size());
    for (size_t i = 0; i < buffers.size(); i++)
    {
        vk::ImageViewCreateInfo color_attachment_view
        (
            {},
            images[i],
            vk::ImageViewType::e2D,
            swapchain_info.surface_format.format,
            {},
            { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        );

        auto image_view = device->createImageView(color_attachment_view);
        vk::ImageView attachments[] = { image_view, depth_buffer.view };

        vk::FramebufferCreateInfo framebuffer_info
        (
            {},
            context->renderpass,
            2,
            attachments,
            swapchain_info.extent.width,
            swapchain_info.extent.height,
            1
        );

        buffers[i].image = images[i];
        buffers[i].view = device->createImageView(color_attachment_view);
        buffers[i].framebuffer = device->createFramebuffer(framebuffer_info);
        buffers[i].device = &context->device.get();
    }
}

vk::Framebuffer VkWindow::get_framebuffer(int index) const
{
    return buffers[index].framebuffer;
}

void VkWindow::end_frame()
{
    vk::Result result;
    auto& device = context->device;

    // Finish recording
    auto& command_buffer = context->get_command_buffer();
    command_buffer.endRenderPass();
    command_buffer.end();

    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    submit_info = vk::SubmitInfo();
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_semaphores[current_frame];
    submit_info.pWaitDstStageMask = waitStages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &context->command_buffers[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_semaphores[current_frame];

    context->graphics_queue.submit(submit_info, flight_fences[current_frame]);

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores =  &render_semaphores[current_frame];

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &image_index;

    //vk::Result result;
    try
    {
        result = present_queue.presentKHR(presentInfo);
    }
    catch (vk::OutOfDateKHRError err)
    {
        result = vk::Result::eErrorOutOfDateKHR;
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eSuboptimalKHR || framebuffer_resized)
    {
        framebuffer_resized = false;
        // recreate_swapchain();
        return;
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VkWindow::acquire_next_image(vk::Semaphore semaphore, uint32_t image_index)
{
    auto& device = context->device;
    auto result = device->acquireNextImageKHR(swapchain, UINT64_MAX, semaphore, vk::Fence());
    image_index = result.value;
}

void VkWindow::create_depth_buffer()
{
    auto& device = context->device;

    // Create an optimal image used as the depth stencil attachment
    vk::ImageCreateInfo image
    (
        {},
        vk::ImageType::e2D,
        swapchain_info.depth_format,
        vk::Extent3D(swapchain_info.extent, 1),
        1, 1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment
    );

    depth_buffer.image = device->createImage(image);

    // Allocate memory for the image (device local) and bind it to our image
    auto requirements = device->getImageMemoryRequirements(depth_buffer.image);
    auto memory_type_index = Buffer::find_memory_type(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, context);
    vk::MemoryAllocateInfo memory_alloc(requirements.size, memory_type_index);

    depth_buffer.memory = device->allocateMemory(memory_alloc);
    device->bindImageMemory(depth_buffer.image, depth_buffer.memory, 0);

    // Create a view for the depth stencil image
    vk::ImageViewCreateInfo depth_view
    (
        {},
        depth_buffer.image,
        vk::ImageViewType::e2D,
        swapchain_info.depth_format,
        {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1)
    );
    depth_buffer.view = device->createImageView(depth_view);
}

SwapchainInfo VkWindow::get_swapchain_info() const
{
    SwapchainInfo info;
    auto& physical_device = context->physical_device;

    // Choose surface format
    auto formats = physical_device.getSurfaceFormatsKHR(surface);
    info.surface_format = formats[0];

    if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
    {
        info.surface_format = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }
    else
    {
        for (const auto& format : formats)
        {
            if (format.format == vk::Format::eB8G8R8A8Unorm &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                info.surface_format = format;
                break;
            }
        }
    }

    // Choose best present mode
    auto present_modes = physical_device.getSurfacePresentModesKHR(surface);
    info.present_mode = vk::PresentModeKHR::eFifo;

    // Query surface capabilities
    auto capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
    info.extent = capabilities.currentExtent;

    if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
        extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

        info.extent = extent;
    }

    // Find a suitable depth (stencil) format that is supported by the device
    auto depth_formats = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint,
                           vk::Format::eD32Sfloat, vk::Format::eD16Unorm };
    info.depth_format = vk::Format::eUndefined;

    for (auto& format : depth_formats)
    {
        auto format_props = physical_device.getFormatProperties(format);
        auto search = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
        if ((format_props.optimalTilingFeatures & search) == search)
        {
            info.depth_format = format;
            break;
        }
    }

    if (info.depth_format == vk::Format::eUndefined)
        throw std::runtime_error("[VK] Couldn't find optinal depth format");

    // Determine the number of images
    info.image_count = capabilities.minImageCount + 1 > capabilities.maxImageCount &&
                       capabilities.maxImageCount > 0 ?
                       capabilities.maxImageCount :
                       capabilities.minImageCount + 1;

    return info;
}

void VkWindow::create_sync_objects()
{
    auto& device = context->device;

    image_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        image_semaphores[i] = device->createSemaphore({});
        render_semaphores[i] = device->createSemaphore({});
        flight_fences[i] = device->createFence({ vk::FenceCreateFlagBits::eSignaled });
    }
}

bool VkWindow::queue_present(vk::Queue queue, uint32_t image_index, vk::Semaphore wait_semaphore)
{
    vk::PresentInfoKHR present_info({}, {}, 1, &swapchain, &image_index);
    if (wait_semaphore)
    {
        present_info.pWaitSemaphores = &wait_semaphore;
        present_info.waitSemaphoreCount = 1;
    }

    return queue.presentKHR(present_info) == vk::Result::eSuccess;
}
