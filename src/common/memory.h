#pragma once
#include <cstdint>

/*  Each KUSEG region is 0x1FFFFFFF (512MB) in length and
    contains the RAM (32MB) and mirrors of the RAM (cached, uncached, accelerated) 
	+ other things like some EE registers/VU memory/Timers. The user
    KUSEG extends up to 0x7FFFFFFF (2048MB) which doesn't
    seem to be used by anything (scratchpad is at 0x70000000 though)
*/
constexpr uint32_t MEMORY_RANGE = 512 * 1024 * 1024;

constexpr uint32_t KUSEG_MASKS[8] = {
	/* KUSEG: Don't touch the address, it's fine */
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	/* KSEG0: Strip the MSB (0x8 -> 0x0 and 0x9 -> 0x1) */
	0x7fffffff,
	/* KSEG1: Strip the 3 MSB's (0xA -> 0x0 and 0xB -> 0x1) */
	0x1fffffff,
	/* KSEG2: Don't touch the address, it's fine */
	0xffffffff, 0xffffffff,
};

/* Just a simple nice wrapper over memory ranges, with constexpr so no overhead */
struct Range {
	constexpr Range(uint32_t begin, uint64_t size) :
		start(begin), length(size) {}

	inline bool contains(uint32_t addr) const 
	{
		return (addr >= start && addr < start + length);
	}

	inline uint32_t offset(uint32_t addr) const 
	{
		return addr - start;
	}

	uint32_t start = 0; 
	uint64_t length = 0;
};

/* EE memory ranges */
namespace ee
{
	constexpr Range RAM = Range(0x00000000, 32 * 1024 * 1024LL);
	constexpr Range IO_REGISTERS = Range(0x10000000, 64 * 1024LL);
	constexpr Range VU_MEMORY = Range(0x11000000, 40 * 1024LL);
	constexpr Range GS_PRIV_REGS = Range(0x12000000, 8 * 1024LL);
	constexpr Range IOP_RAM = Range(0x1c000000, 2 * 1024 * 1024LL);
	constexpr Range BIOS = Range(0x1fc00000, 4 * 1024 * 1024LL);
};

/* IOP memory ranges */
namespace iop
{
	constexpr Range RAM = Range(0x1c000000, 2 * 1024 * 1024LL);
	constexpr Range KSEG = Range(0x00000000, 2048 * 1024 * 1024LL);
	constexpr Range KSEG0 = Range(0x80000000, 512 * 1024 * 1024LL);
	constexpr Range KSEG1 = Range(0xA0000000, 512 * 1024 * 1024LL);
	constexpr Range KSEG2 = Range(0xC0000000, 1024 * 1024 * 1024LL);
	constexpr Range DMA1 = Range(0x1f801080, 0x80LL), DMA2 = Range(0x1f801500, 0x80LL);
	constexpr Range TIMERS1 = Range(0x1f801100, 0x30), TIMERS2 = Range(0x1f801480, 0x30);
	constexpr Range INTERRUPT = Range(0x1f801070, 0xC);
};