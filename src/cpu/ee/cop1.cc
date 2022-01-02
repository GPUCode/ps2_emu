#include <cpu/ee/cop1.h>
#include <cpu/ee/ee.h>

namespace ee
{
	void COP1::execute(Instruction instr)
	{
		uint16_t function = instr.r_type.funct;
		switch (function)
		{
		case 0b011000:
			op_adda(instr); break;
		default:
			fmt::print("[COP1] Unimplemented opcode: {:#08b}\n", function);
			std::abort();
		}
	}

	void COP1::op_adda(Instruction instr)
	{
		uint16_t fs = instr.r_type.rd;
		uint16_t ft = instr.r_type.rt;

		fmt::print("[COP1] ADDA.S: ACC = FPR[{}] ({}) + FPR[{}] ({})\n", fs, fpr[fs].fint, ft, fpr[ft].fint);
		acc.fint = fpr[fs].fint + fpr[ft].fint;
	}
}