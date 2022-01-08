#pragma once
#include <cstdint>

namespace ee
{
	constexpr uint32_t COP1_OPCODE = 0b10001;

	union FPR
	{
		uint32_t uint = 0;
		float fint;
	};

	union FCR0
	{
		uint32_t value = 0;
		struct
		{
			uint32_t revision : 8;
			uint32_t impl : 8;
			uint32_t : 16;
		};
	};

	union FCR31
	{
		uint32_t value = 0;
		struct
		{
			uint32_t rounding_mode : 2;
			uint32_t flags : 5;
			uint32_t enable : 5;
			uint32_t cause : 6;
			uint32_t : 5;
			uint32_t condition : 1;
			uint32_t flush_denormals : 1;
			uint32_t : 7;
		};
	};

	struct Instruction;
	struct COP1
	{
		void execute(Instruction instr);
		void op_adda(Instruction instr);
		void op_madd(Instruction instr);

		FPR fpr[32];
		FCR0 fcr0;
		FCR31 fcr31;
		FPR acc;
	};
}