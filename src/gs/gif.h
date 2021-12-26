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

	struct GIF : public common::Component
	{
		GIF(common::Emulator* parent);
		
		uint64_t read(uint32_t addr);
		void write(uint32_t addr, uint64_t data);

	private:
		common::Emulator* emulator;
		GIFRegs regs = {};
	};
}