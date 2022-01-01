#include <cpu/vu/vu0.h>
#include <cpu/ee/ee.h>
#include <fmt/core.h>

namespace vu
{
	VU0::VU0(ee::EmotionEngine* parent) :
		cpu(parent)
	{
		/* W = 1.0f on VF00 */
		regs.vf[0].w = 1.0f;
	}

	void VU0::special1(ee::Instruction instr)
	{
		uint32_t function = instr.value & 0x3f;
		VUInstr vu_instr = { .value = instr.value };
		switch (function)
		{
		case 0b101100:
			return op_vsub(vu_instr);
		case 0b110000:

		case 0b111100 ... 0b111111:
			return special2(instr);
		default:
			fmt::print("[VU0] Unimplemented special1 macro operation {:#08b}\n", function);
		}
	}

	void VU0::special2(ee::Instruction instr)
	{
		uint32_t flo = instr.value & 0x3;
		uint32_t fhi = (instr.value >> 6) & 0x1f;
		uint32_t opcode = flo | (fhi * 4);
		
		VUInstr vu_instr = { .value = instr.value };
		switch (opcode)
		{
		case 0b0111111: op_viswr(vu_instr); break;
		case 0b0110101: op_vsqi(vu_instr); break;
		default:
			fmt::print("[VU0] Unimplemented special2 macro operation {:#09b}\n", opcode);
		}
	}

	void VU0::op_cfc2(ee::Instruction instr)
	{
		uint16_t id = instr.r_type.rd;
		uint16_t rt = instr.r_type.rt;
		auto ptr = (uint32_t*)&regs + id;
		 
		cpu->gpr[rt].dword[0] = (int32_t)*ptr;
		fmt::print("[VU0] CFC2: GPR[{}] = VI[{}] ({:#x})\n", rt, id, *ptr);
	}
	
	void VU0::op_ctc2(ee::Instruction instr)
	{
		uint16_t id = instr.r_type.rd;
		uint16_t rt = instr.r_type.rt;
		auto ptr = (uint32_t*)&regs + id;

		*ptr = cpu->gpr[rt].word[0];
		fmt::print("[VU0] CTC2: VI[{}] = GPR[{}] ({:#x})\n", id, rt, *ptr);
	}

	void VU0::op_qmfc2(ee::Instruction instr)
	{
		uint16_t fd = instr.r_type.rd;
		uint16_t rt = instr.r_type.rt;

		cpu->gpr[rt].qword = regs.vf[fd].qword;
		uint64_t upper = (regs.vf[fd].qword >> 64);
		fmt::print("[VU0] QMFC2: GPR[{}] = VF[{}] ({:#x}{:016x})\n", rt, fd, upper, (uint64_t)regs.vf[fd].qword);
	}

	void VU0::op_qmtc2(ee::Instruction instr)
	{
		uint16_t fd = instr.r_type.rd;
		uint16_t rt = instr.r_type.rt;

		regs.vf[fd].qword = cpu->gpr[rt].qword;
		uint64_t upper = (cpu->gpr[rt].qword >> 64);
		fmt::print("[VU0] QMTC2:  VF[{}] = GPR[{}] ({:#x}{:016x})\n", fd, rt, upper, (uint64_t)cpu->gpr[rt].qword);
	}

	void VU0::op_viswr(VUInstr instr)
	{
		uint16_t is = instr.is;
		uint16_t it = instr.it;

		/* VI[is] contains the address divided by 16 */
		uint32_t address = regs.vi[is] * 16;
		auto ptr = (uint32_t*)&data[address];

		fmt::print("[VU0] VISWR Writing VI[{}] = {:#x} to address {:#x} (", it, regs.vi[it], address);
		for (int i = 0; i < 4; i++)
		{
			/* If the component is set in the dest mask */
			if (instr.dest & (1 << i))
			{
				fmt::print("{}, ", "XYZW"[i]);
				*(ptr + i) = regs.vi[it] & 0xffff;
			}
		}
		fmt::print("\b\b)\n");
	}
	
	void VU0::op_vsub(VUInstr instr)
	{
		uint16_t fd = instr.fd;
		uint16_t fs = instr.fs;
		uint16_t ft = instr.ft;

		fmt::print("[VU0] VSUB: VF[{}] = VF[{}] - VF[{}] (", fd, fs, ft);
		for (int i = 0; i < 4; i++)
		{
			/* If the component is set in the dest mask */
			if (instr.dest & (1 << i))
			{
				fmt::print("{}, ", "XYZW"[i]);
				regs.vf[fd].x = regs.vf[fs].x - regs.vf[ft].x;
			}
		}
		fmt::print("\b\b)\n");
	}
	
	void VU0::op_vsqi(VUInstr instr)
	{
		uint16_t fs = instr.fs;
		uint16_t it = instr.it;

		/* VI[is] contains the address divided by 16 */
		uint32_t address = regs.vi[it] * 16;
		auto ptr = (uint32_t*)&data[address];

		fmt::print("[VU0] VSQI Writing VF[{}] = {:#x} to address {:#x} (", fs, (uint64_t)regs.vf[fs].qword, address);
		for (int i = 0; i < 4; i++)
		{
			/* If the component is set in the dest mask */
			if (instr.dest & (1 << i))
			{
				fmt::print("{}, ", "XYZW"[i]);
				*(ptr + i) = regs.vf[fs].word[i];
			}
		}
		fmt::print("\b\b)\n");
		regs.vi[it]++;
	}
	
	void VU0::op_viadd(VUInstr instr)
	{
		uint16_t id = instr.id;
		uint16_t is = instr.is;
		uint16_t it = instr.it;

		fmt::print("[VU0] VIADD VI[{}] = VI[{}] ({:#x}) + VI[{}] ({:#x})\n", id, is, regs.vi[is], it, regs.vi[it]);
		regs.vi[id] = regs.vi[is] + regs.vi[it];
	}
}