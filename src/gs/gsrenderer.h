#pragma once
#include <cstdint>

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

	struct Vert
	{
		int x, y, z;
	};

	/* The main renderer responsible for drawing the fancy
	   graphics to the screen. */
	struct GSRenderer
	{
		GSRenderer();

		void render();
		void submit_sprite(Vert v1, Vert v2);

	private:
		uint32_t vbo, vao;
	};
}