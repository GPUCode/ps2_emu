#pragma once

/* Just a simple nice wrapper over memory ranges, with constexpr so no overhead */
struct Range {
	constexpr Range(unsigned int begin, unsigned long size) :
		start(begin), length(size) {}

	inline bool contains(unsigned int addr) const 
	{
		return (addr >= start && addr < start + length);
	}

	inline uint offset(unsigned int addr) const 
	{
		return addr - start;
	}

	unsigned int start = 0; 
	unsigned long length = 0;
};

/* Memory ranges. */
constexpr Range RAM = Range(0x00000000, 32 * 1024 * 1024LL);
constexpr Range IO_REGISTERS = Range(0x10000000, 64 * 1024LL);
constexpr Range VU_MEMORY = Range(0x11000000, 40 * 1024LL);
constexpr Range GS_PRIV_REGS = Range(0x12000000, 8 * 1024LL);
constexpr Range IOP_RAM = Range(0x1c000000, 2 * 1024 * 1024LL);
constexpr Range BIOS = Range(0x1fc00000, 4 * 1024 * 1024LL);