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
	union Vector
	{
		/* The unsigned part is used to preserve bit data */
		uint128_t qword;
		uint32_t word[4];
		/* Float part is used for float operations */
		float fword[4];
		struct { float x, y, z, w; };
	};

	struct Registers
	{
		uint32_t vi[16];
		uint32_t control[16];
		Vector vf[32];
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
	};

	struct VU0
	{
		VU0(ee::EmotionEngine* parent);
		~VU0() = default;

		/* Read/Write VU memory */
		template <typename T>
		T read(uint32_t addr);

		template <typename T>
		void write(uint32_t addr, T value);

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
		void op_vsuba(VUInstr instr);
		void op_vmadda(VUInstr instr);
		void op_vmsuba(VUInstr instr);
		void op_vitof0(VUInstr instr);

	private:
		ee::EmotionEngine* cpu;
		Registers regs = {};
		Vector acc;

		/* VU0 memory */
		uint8_t data[4 * 1024] = {};
		uint8_t code[4 * 1024] = {};
	};
	
	template<typename T>
	inline T VU0::read(uint32_t addr)
	{
		if (addr < 0x11004000)
		{
			return *(T*)&code[addr & 0x1000];
		}
		else
		{
			return *(T*)&data[addr & 0x1000];
		}
	}
	
	template<typename T>
	inline void VU0::write(uint32_t addr, T value)
	{
		if (addr < 0x11004000)
		{
			*(T*)&code[addr & 0x1000] = value;
		}
		else
		{
			*(T*)&data[addr & 0x1000] = value;
		}
	}
}