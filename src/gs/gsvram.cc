#include <gs/gsvram.h>

namespace gs
{
	/* Block/Column/Pixel parameters for each format */
	template <>
	struct FormatInfo<PSMCT32>
	{
		constexpr static uint16_t PAGE_PIXEL_WIDTH = 64;
		constexpr static uint16_t PAGE_PIXEL_HEIGHT = 32;
		constexpr static uint16_t PAGE_BLOCK_WIDTH = 8;
		constexpr static uint16_t PAGE_BLOCK_HEIGHT = 4;
		constexpr static uint16_t BLOCK_PIXEL_WIDTH = 8;
		constexpr static uint16_t BLOCK_PIXEL_HEIGHT = 8;
		constexpr static uint16_t COLUMN_PIXEL_HEIGHT = 2;
		constexpr static int block_layout[4][8] =
		{
			{  0,  1,  4,  5, 16, 17, 20, 21 },
			{  2,  3,  6,  7, 18, 19, 22, 23 },
			{  8,  9, 12, 13, 24, 25, 28, 29 },
			{ 10, 11, 14, 15, 26, 27, 30, 31 }
		};
	};
	
	template <>
	struct FormatInfo<PSMCT16>
	{
		constexpr static uint16_t PAGE_PIXEL_WIDTH = 64;
		constexpr static uint16_t PAGE_PIXEL_HEIGHT = 64;
		constexpr static uint16_t PAGE_BLOCK_WIDTH = 4;
		constexpr static uint16_t PAGE_BLOCK_HEIGHT = 8;
		constexpr static uint16_t BLOCK_PIXEL_WIDTH = 16;
		constexpr static uint16_t BLOCK_PIXEL_HEIGHT = 8;
		constexpr static uint16_t COLUMN_PIXEL_HEIGHT = 2;
		constexpr static int block_layout[8][4] =
		{
			{  0,  2,  8, 10 },
			{  1,  3,  9, 11 },
			{  4,  6, 12, 14 },
			{  5,  7, 13, 15 },
			{ 16, 18, 24, 26 },
			{ 17, 19, 25, 27 },
			{ 20, 22, 28, 30 },
			{ 21, 23, 29, 31 }
		};
	};

	void Page::write_psmct32(uint16_t x, uint16_t y, uint32_t value)
	{
		using Info = FormatInfo<PSMCT32>;

		/* Get the coords of the curerent block inside the page */
		uint16_t block_x = (x / Info::BLOCK_PIXEL_WIDTH) % Info::PAGE_BLOCK_WIDTH;
		uint16_t block_y = (y / Info::BLOCK_PIXEL_HEIGHT) % Info::PAGE_BLOCK_HEIGHT;

		/* Blocks in a page are not stored linearly so use small
		   lookup table to figure out the offset of the block in memory */
		uint16_t block = Info::block_layout[block_y][block_x] % BLOCKS_PER_PAGE;
		uint32_t column = (y / Info::COLUMN_PIXEL_HEIGHT) % COLUMNS_PER_BLOCK;

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

	void Page::write_psmct16(uint16_t x, uint16_t y, uint16_t value)
	{
		using Info = FormatInfo<PSMCT16>;

		/* Get the coords of the curerent block inside the page */
		uint16_t block_x = (x / Info::BLOCK_PIXEL_WIDTH) % Info::PAGE_BLOCK_WIDTH;
		uint16_t block_y = (y / Info::BLOCK_PIXEL_HEIGHT) % Info::PAGE_BLOCK_HEIGHT;

		/* Blocks in a page are not stored linearly so use small
		   lookup table to figure out the offset of the block in memory */
		uint16_t block = Info::block_layout[block_y][block_x] % BLOCKS_PER_PAGE;
		uint32_t column = (y / Info::COLUMN_PIXEL_HEIGHT) % COLUMNS_PER_BLOCK;

		constexpr static int pixels[2][8] =
		{
			{ 0, 1, 4, 5,  8,  9, 12, 13 },
			{ 2, 3, 6, 7, 10, 11, 14, 15 }
		};

		/* Pixels in columns are not stored linearly either... */
		uint32_t pixel = pixels[y & 0x1][(x >> 1) % 8];
		uint32_t offset = column * COLUMN_SIZE + pixel * 2;

		*(uint16_t*)&blocks[block][offset] = value;
	}
}