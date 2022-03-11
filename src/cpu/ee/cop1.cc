#include <cpu/ee/cop1.h>
#include <cpu/ee/ee.h>
#include <common/emulator.h>

namespace ee
{
	/* Check for overflow */
	static inline float overflow_check(uint32_t value)
	{
		if ((value & 0x7F800000) == 0x7F800000)
			value = (value & 0x80000000) | 0x7F7FFFFF;
		return *(float*)&value;
	}

	void COP1::execute(Instruction instr)
	{
		uint16_t function = instr.r_type.funct;
		switch (function)
		{
		case 0b011000: op_adda(instr); break;
		case 0b011100: op_madd(instr); break;
		default:
            common::Emulator::terminate("[COP1] Unimplemented opcode: {:#08b}\n", function);
		}
	}

	void COP1::op_adda(Instruction instr)
	{
		uint16_t fs = instr.r_type.rd;
		uint16_t ft = instr.r_type.rt;

		fmt::print("[COP1] ADDA.S: ACC = FPR[{}] ({}) + FPR[{}] ({})\n", fs, fpr[fs].fint, ft, fpr[ft].fint);
		acc.fint = fpr[fs].fint + fpr[ft].fint;
	}
	
	void COP1::op_madd(Instruction instr)
	{
		uint16_t fs = instr.r_type.rd;
		uint16_t ft = instr.r_type.rt;
		uint16_t fd = instr.r_type.sa;

		float reg1 = overflow_check(fpr[fs].uint);
		float reg2 = overflow_check(fpr[ft].uint);
		float accumulator = overflow_check(acc.uint);
		fpr[fs].fint = accumulator + (reg1 * reg2);
		
		fmt::print("[COP1] MADD.s: ACC ({}) + GPR[{}] ({}) * GPR[{}] ({}) = {}\n", accumulator, fs, reg1, ft, reg2, fpr[fd].fint);
	}
}
