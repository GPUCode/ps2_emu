#pragma once
#include <cstdint>
#include <vector>

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

	struct GSVertex
	{
		float x, y, z = 0;
		float r = 0.0f, g = 0.0f, b = 0.0f;
	};

	/* The main renderer responsible for drawing the fancy
	   graphics to the screen. */
	struct GSRenderer
	{
		GSRenderer();

		void render();

		void submit_vertex(GSVertex v1);
		void submit_sprite(GSVertex v1, GSVertex v2);

	private:
		uint32_t vbo, vao;
		std::vector<GSVertex> draw_data;
	};
}