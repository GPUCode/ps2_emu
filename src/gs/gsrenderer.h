#pragma once
#include <gs/vulkan/buffer.h>
#include <gs/vulkan/texture.h>
#include <cstdint>
#include <memory>
#include <vector>

class VkWindow;
class Buffer;

namespace gs
{
	enum Primitive
	{
		Point = 0,
		Line = 1,
		LineStrip = 2,
		Triangle = 3,
		TriangleStrip = 4,
		TriangleFan = 5,
		Sprite = 6
	};

    using GSVertex = Vertex;

	/* The main renderer responsible for drawing the fancy
	   graphics to the screen. */
	struct GSRenderer
	{
        GSRenderer(VkWindow* window);
        ~GSRenderer();

		void render();
        void set_depth_function(uint32_t test_bits);

		void submit_vertex(GSVertex v1);
		void submit_sprite(GSVertex v1, GSVertex v2);

    public:
        VkWindow* window = nullptr;
		std::vector<GSVertex> draw_data;
        std::unique_ptr<VertexBuffer> buffer;
        std::unique_ptr<VkTexture> vram;
        int vertex_count = 0;
	};
}
