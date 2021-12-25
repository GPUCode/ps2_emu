#pragma once
#include <cstdint>

namespace iop
{
	/* All COP0 instructions have this */
	constexpr uint8_t COP0_OPCODE = 0b010000;

	/* Instruction types */
	constexpr uint8_t COP0_MF0 = 0b00000;
	constexpr uint8_t COP0_MT0 = 0b00100;
	constexpr uint8_t COP0_C0 = 0b10000;
	constexpr uint8_t COP0_TLB = 0b10000;

	union COP0STAT 
	{
		uint32_t value;
		struct 
		{
			uint32_t IEc : 1;		/* Interrupt Enable (current) */
			uint32_t KUc : 1;		/* Kernel-User Mode (current) */
			uint32_t IEp : 1;		/* Interrupt Enable (previous) */
			uint32_t KUp : 1;		/* Kernel-User Mode (previous) */
			uint32_t IEo : 1;		/* Interrupt Enable (old) */
			uint32_t KUo : 1;		/* Kernel-User Mode (old) */
			uint32_t : 2;
			uint32_t Im : 8;	/* Hardware Interrupt Mask */
			uint32_t IsC : 1;		/* Isolate Cache */
			uint32_t : 1;
			uint32_t PZ : 1;		/* Parity Zero */
			uint32_t CM : 1;
			uint32_t PE : 1;		/* Parity Error */
			uint32_t TS : 1;		/* TLB Shutdown */
			uint32_t BEV : 1;		/* Bootstrap Exception Vectors */
			uint32_t : 5;
			uint32_t Cu : 4;		/* Coprocessor Usability */
		};
	};

	union COP0CAUSE 
	{
		uint32_t value;
		struct 
		{
			uint32_t : 2;
			uint32_t excode : 5;	/* Exception Code */
			uint32_t : 1;
			uint32_t IP : 8;		/* Interrupt Pending */
			uint32_t : 12;
			uint32_t CE : 2;		/* Coprocessor Error */
			uint32_t BT : 1;		/* Branch Taken */
			uint32_t BD : 1;		/* Branch Delay */
		};
	};

	union COP0 
	{
		uint32_t regs[32] = {};
		struct 
		{
			uint32_t r0;
			uint32_t r1;
			uint32_t r2;
			uint32_t Bpc;		/* Breakpoint Program Counter */
			uint32_t r4;
			uint32_t BDA;		/* Breakpoint Data Address */
			uint32_t TAR;		/* Target Address */
			uint32_t DCIC;		/* Debug and Cache Invalidate Control */
			uint32_t BadA;		/* Bad Address */
			uint32_t BDAM;		/* Breakpoint Data Address Mask */
			uint32_t r10;
			uint32_t BpcM;		/* Breakpoint Program Counter Mask */
			COP0STAT sr;	    /* Status */
			COP0CAUSE cause;	/* Cause */
			uint32_t epc;		/* Exception Program Counter */
			uint32_t PRId;		/* Processor Revision Identifier */
		};
	};
};