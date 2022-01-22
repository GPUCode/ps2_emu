#pragma once
#include <cstdint>

namespace gs
{
	/* GS Memory constructs */
	constexpr uint16_t PAGE_SIZE = 8192;
	constexpr uint16_t BLOCK_SIZE = 256;
	constexpr uint16_t BLOCKS_PER_PAGE = 32;

	/* Block dimentations */
	constexpr uint16_t BLOCK_PIXEL_WIDTH = 8;
	constexpr uint16_t BLOCK_PIXEL_HEIGHT = 8;

	/* Column definition */
	constexpr uint16_t COLUMN_PIXEL_HEIGHT = 2;
	constexpr uint16_t COLUMNS_PER_BLOCK = 4;
	constexpr uint16_t COLUMN_SIZE = 64;

	struct Page
	{
		/* Page dimentions */
		constexpr static uint16_t PIXEL_WIDTH = 64;
		constexpr static uint16_t PIXEL_HEIGHT = 32;
		constexpr static uint16_t BLOCK_WIDTH = 8;
		constexpr static uint16_t BLOCK_HEIGHT = 4;

		/* Write a 32bit value to a PSMCT32 buffer */
		void write_psmct32(uint16_t x, uint16_t y, uint32_t value)
		{
			/* Get the coords of the curerent block inside the page */
			uint16_t block_x = (x / BLOCK_PIXEL_WIDTH) % BLOCK_WIDTH;
			uint16_t block_y = (y / BLOCK_PIXEL_HEIGHT) % BLOCK_HEIGHT;

			/* Block layout in the page */
			constexpr static int block_layout[4][8] =
			{
				{  0,  1,  4,  5, 16, 17, 20, 21 },
				{  2,  3,  6,  7, 18, 19, 22, 23 },
				{  8,  9, 12, 13, 24, 25, 28, 29 },
				{ 10, 11, 14, 15, 26, 27, 30, 31 }
			};

			/* Blocks in a page are not stored linearly so use small
			   lookup table to figure out the offset of the block in memory */
			uint16_t block = block_layout[block_y][block_x] % BLOCKS_PER_PAGE;

			uint32_t column = (y / COLUMN_PIXEL_HEIGHT) % COLUMNS_PER_BLOCK;

			constexpr static int pixels[2][8]
			{
				{ 0, 1, 4, 5,  8,  9, 12, 13 },
				{ 2, 3, 6, 7, 10, 11, 14, 15 }
			};
			/* Pixels in columns are not stored linearly either... */
			uint32_t pixel = pixels[y & 0x1][x % 8];
			uint32_t offset = column * COLUMN_SIZE + pixel * 4;

			*(uint32_t*)&blocks[block][offset] = value;
		}

	private:
		uint8_t blocks[BLOCKS_PER_PAGE][BLOCK_SIZE] = {};
	};
}