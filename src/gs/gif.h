#pragma once
#include <common/component.h>

namespace common
{
	class Emulator;
}

namespace gs
{
	union GIFSTAT
	{
		uint32_t value;
		struct
		{
			uint32_t M3R : 1;
			uint32_t M3P : 1;
			uint32_t IMT : 1;
			uint32_t PSE : 1;
			uint32_t IP3 : 1;
			uint32_t P3Q : 1;
			uint32_t P2Q : 1;
			uint32_t P1Q : 1;
			uint32_t OPH : 1;
			uint32_t APATH : 2;
			uint32_t DIR : 1;
			uint32_t : 12;
			uint32_t FQC : 5;
			uint32_t : 3;
		};
	};

	struct GIFRegs
	{
		uint32_t GIF_CTRL;
		uint32_t GIF_MODE;
		GIFSTAT GIF_STAT;
		uint32_t GIF_TAG0;
		uint32_t GIF_TAG1;
		uint32_t GIF_TAG2;
		uint32_t GIF_TAG3;
		uint32_t GIF_CNT;
		uint32_t GIF_P3CNT;
		uint32_t GIF_P3TAG;
	};

	constexpr uint16_t GIF_TAG_NULL = -1;

	/* The header of each GS Primitive */
	union GIFTag
	{
		uint128_t value = GIF_TAG_NULL;
		struct
		{
			uint128_t nloop : 15;
			uint128_t eop : 1;
			uint128_t : 30;
			uint128_t pre : 1;
			uint128_t prim : 11;
			uint128_t flg : 2;
			uint128_t nreg : 4;
			uint128_t regs : 64;
		};
	};

	enum REGDesc : uint64_t
	{
		PRIM = 0,
		RGBAQ = 1,
		ST = 2,
		UV = 3,
		XYZF2 = 4, 
		XYZ2 = 5,
		TEX0_1 = 6,
		TEX0_2 = 7,
		CLAMP_1 = 8,
		CLAMP_2 = 9,
		FOG = 10,
		XYZF3 = 12,
		XYZ3 = 13,
		A_D = 14,
		NOP = 15
	};

	enum Format : uint32_t
	{
		PACKED = 0,
		REGLIST = 1,
		IMAGE = 2,
		Disable = 3
	};

	struct GIF : public common::Component
	{
		GIF(common::Emulator* parent);
		
		uint32_t read(uint32_t addr);
		void write(uint32_t addr, uint32_t data);

		/* Writes from various PATHs */
		void write_path3(uint32_t addr, uint128_t data);

	private:
		void process_packed(uint128_t qword);

	private:
		common::Emulator* emulator;
		GIFRegs regs = {};
		
		/* Used during transfers */
		GIFTag tag;
		int data_count = 0, reg_count = 0;
	};
}