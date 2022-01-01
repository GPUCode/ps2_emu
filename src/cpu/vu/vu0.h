#pragma once
#include <cstdint>

namespace ee
{
	class EmotionEngine;
	struct Instruction;
}

using uint128_t = unsigned __int128;

namespace vu
{
	union VF
	{
		uint128_t qword;
		uint32_t word[4];
		struct { float x, y, z, w; };
	};

	struct Registers
	{
		uint32_t vi[16];
		uint32_t control[16];
		VF vf[32];
	};

	union VUInstr
	{
		uint32_t value;
		struct
		{
			uint32_t : 6;
			uint32_t id : 5;
			uint32_t is : 5;
			uint32_t it : 5;
			/* This acts as a bit mask that tells the CPU which fields
			   to write (X, Y, Z, W) */
			uint32_t dest : 4;
			uint32_t : 7;
		};
		struct
		{
			uint32_t : 6;
			uint32_t fd : 5;
			uint32_t fs : 5;
			uint32_t ft : 5;
			uint32_t : 11;
		};

		VUInstr operator=(uint32_t i)
		{
			VUInstr v = { .value = i };
			return v;
		}
	};

	struct VU0
	{
		VU0(ee::EmotionEngine* parent);
		~VU0() = default;

		/* There are executed directly from the EE
		   when VU0 is in macro mode */
		void special1(ee::Instruction instr);
		void special2(ee::Instruction instr);		
		void op_cfc2(ee::Instruction instr);
		void op_ctc2(ee::Instruction instr);
		void op_qmfc2(ee::Instruction instr);
		void op_qmtc2(ee::Instruction instr);

		void op_viswr(VUInstr instr);
		void op_vsub(VUInstr instr);
		void op_vsqi(VUInstr instr);
		void op_viadd(VUInstr instr);

	private:
		ee::EmotionEngine* cpu;
		Registers regs = {};

		uint8_t data[4 * 1024] = {};
	};
}