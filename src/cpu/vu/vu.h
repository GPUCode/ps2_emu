#pragma once
#include <cpu/vu/common.h>

namespace ee
{
	class EmotionEngine;
	struct Instruction;
}

namespace vu
{
	struct VectorUnit
	{
		VectorUnit(ee::EmotionEngine* parent);
		~VectorUnit() = default;

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
		void op_vadd(VUInstr instr);
		void op_vmr32(VUInstr instr);

		Registers regs = {};
		Vector acc;

		/* VU0 normally has only 4KB of memory, but making it
		  16KB makes it easier to unify the class */
		uint8_t data[16 * 1024] = {};
		uint8_t code[16 * 1024] = {};

	private:
		ee::EmotionEngine* cpu;
	};

	template<typename T>
	inline T VectorUnit::read(uint32_t addr)
	{
		if (addr < 0x1100c000)
		{
			return *(T*)&code[addr & 0x3fff];
		}
		else
		{
			return *(T*)&data[addr & 0x3fff];
		}
	}

	template<typename T>
	inline void VectorUnit::write(uint32_t addr, T value)
	{
		if (addr < 0x1100c000)
		{
			*(T*)&code[addr & 0x3fff] = value;
		}
		else
		{
			*(T*)&data[addr & 0x3fff] = value;
		}
	}
}