#pragma once
#include <cstdint>

namespace gs
{
	/* Common GS memory constructs */
	constexpr uint16_t PAGE_SIZE = 8192;
	constexpr uint16_t BLOCKS_PER_PAGE = 32;
	constexpr uint16_t BLOCK_SIZE = 256;
	constexpr uint16_t COLUMNS_PER_BLOCK = 4;
	constexpr uint16_t COLUMN_SIZE = 64;

	enum PixelFormat : uint8_t
	{
		PSMCT32 = 0x0,
		PSMCT24 = 0x1,
		PSMCT16 = 0x2,
		PSMCT16S = 0xa,
		PSMCT8 = 0x13,
		PSMCT4 = 0x14,
		PSMCT8H = 0x1b,
		PSMCT4HL = 0x24,
		PSMCT4HH = 0x2c,
		PSMZ32 = 0x30,
		PSMZ24 = 0x31,
		PSMZ16 = 0x32,
		PSMZ16S = 0x3a
	};

	/* This contains info about each pixel format. */
	/* It is specialized per format */
	template <PixelFormat fmt>
	struct FormatInfo;

	struct Page
	{
		/* Page dimentions */
		constexpr static uint16_t PIXEL_WIDTH = 64;
		constexpr static uint16_t PIXEL_HEIGHT = 32;
		constexpr static uint16_t BLOCK_WIDTH = 8;
		constexpr static uint16_t BLOCK_HEIGHT = 4;

		/* Write a 32bit value to a PSMCT32 buffer */
		void write_psmct32(uint16_t x, uint16_t y, uint32_t value);

		/* Write a 16bit value to a PSMCT16 buffer */
		void write_psmct16(uint16_t x, uint16_t y, uint16_t value);

	private:
		uint8_t blocks[BLOCKS_PER_PAGE][BLOCK_SIZE] = {};
	};
}