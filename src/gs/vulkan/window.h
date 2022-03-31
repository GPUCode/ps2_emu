#pragma once
#include <gs/vulkan/common.h>
#include <string_view>
#include <memory>

class VkContext;
struct GLFWwindow;

struct SwapchainBuffer
{
    ~SwapchainBuffer()
    {
        device->destroyImageView(view);
        device->destroyFramebuffer(framebuffer);
    }

    vk::Image image;
    vk::ImageView view;
    vk::Framebuffer framebuffer;
    vk::Device* device;
};

struct SwapchainInfo
{
    vk::Format depth_format;
    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    uint32_t image_count;
};

struct DepthBuffer : public NonCopyable
{
    ~DepthBuffer()
    {
        // Destroy depth buffer
        device.destroyImage(image);
        device.destroyImageView(view);
        device.freeMemory(memory);
    }

    vk::Device device;
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
};

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

class VkWindow
{
public:
    VkWindow(int width, int height, std::string_view name);
    ~VkWindow();

    std::shared_ptr<VkContext> create_context(bool validation = true);
    bool should_close() const;
    vk::Extent2D get_extent() const;

    void begin_frame();
    void end_frame();

    void destroy();
    vk::Framebuffer get_framebuffer(int index) const;

private:
    void create_sync_objects();
    void create_swapchain(bool enable_vsync = false);
    SwapchainInfo get_swapchain_info() const;
    void create_depth_buffer();
    void create_present_queue();

public:
    // Window attributes
    GLFWwindow* window;
    uint32_t width = 0, height = 0;
    bool framebuffer_resized = false;
    std::string_view name;

    // Context
    std::shared_ptr<VkContext> context;
    vk::Queue present_queue;

    // Swapchain objects
    vk::UniqueSurfaceKHR surface;
    vk::UniqueSwapchainKHR swapchain;
    std::vector<SwapchainBuffer> buffers;
    SwapchainInfo swapchain_info;
    uint32_t current_frame = 0, image_index = 0;
    uint32_t draw_batch = 0;

    // Depth buffer
    DepthBuffer depth_buffer;

    // Synchronization
    vk::SubmitInfo submit_info;
    std::vector<vk::UniqueSemaphore> image_semaphores;
    std::vector<vk::UniqueSemaphore> render_semaphores;
    std::vector<vk::UniqueFence> flight_fences;
};
