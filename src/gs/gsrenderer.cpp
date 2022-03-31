#include <gs/gsrenderer.h>
#include <common/emulator.h>
#include <gs/vulkan/context.h>
#include <gs/vulkan/window.h>

constexpr uint32_t MAX_VERTICES = 1024 * 1024;

namespace gs
{
    GSRenderer::GSRenderer(VkWindow* window) :
        window(window)
	{
        auto context = window->create_context();

        // Create VRAM texture and vertex buffer
        vram = std::make_unique<VkTexture>(context);
        buffer = std::make_unique<VertexBuffer>(context);

        // Configure texture
        vram->create(32768, 1, vk::ImageType::e1D, vk::Format::eR32Uint);
        buffer->create(MAX_VERTICES);

        // Create vulkan graphics pipeline
        PipelineLayoutInfo info(context);
        info.add_shader_module("shaders/vertex.glsl.spv", vk::ShaderStageFlagBits::eVertex);
        info.add_shader_module("shaders/fragment.glsl.spv", vk::ShaderStageFlagBits::eFragment);
        info.add_resource(vram.get(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 0);

        // Construct graphics pipeline
        context->create_graphics_pipeline(info);

	}
    
    GSRenderer::~GSRenderer()
    {
    }

    void GSRenderer::set_depth_function(uint32_t test_bits)
    {
        auto& context = window->context;

        // Set depth function
        auto& command_buffer = context->get_command_buffer();
        switch (test_bits)
        {
        case 0:
            command_buffer.setDepthCompareOp(vk::CompareOp::eNever);
            break;
        case 1:
            command_buffer.setDepthCompareOp(vk::CompareOp::eGreaterOrEqual);
            break;
        case 3:
            command_buffer.setDepthCompareOp(vk::CompareOp::eGreater);
            break;
        default:
            common::Emulator::terminate("[GS] Unknown depth function selected!\n");
        }
    }

    void GSRenderer::render()
    {
        // Update vertex buffer
        if (!draw_data.empty())
        {
            // Flush renderer
            auto& command_buffer = window->context->get_command_buffer();

            buffer->bind(command_buffer);
            command_buffer.draw(vertex_count, 1, draw_data.size() - vertex_count, 0);
            vertex_count = 0;

            // Copy the vertices to the GPU
            buffer->copy_vertices(draw_data.data(), draw_data.size());
            draw_data.clear();
        }
    }
    
    void GSRenderer::submit_vertex(GSVertex v1)
    {
        draw_data.push_back(v1);
        vertex_count++;
    }

    void GSRenderer::submit_sprite(GSVertex v1, GSVertex v2)
    {
        draw_data.emplace_back(glm::vec3(v2.position.x, v1.position.y, 0));
        draw_data.emplace_back(glm::vec3(v2.position.x, v2.position.y, 0));
        draw_data.emplace_back(glm::vec3(v1.position.x, v1.position.y, 0));
        draw_data.emplace_back(glm::vec3(v2.position.x, v2.position.y, 0));
        draw_data.emplace_back(glm::vec3(v1.position.x, v2.position.y, 0));
        draw_data.emplace_back(glm::vec3(v1.position.x, v1.position.y, 0));

        vertex_count += 6;
    }
}
