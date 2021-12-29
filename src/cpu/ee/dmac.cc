#include <cpu/ee/dmac.h>
#include <common/emulator.h>
#include <cassert>

inline uint32_t get_channel(uint32_t value)
{
	switch (value)
	{
	case 0x80: return 0;
	case 0x90: return 1;
	case 0xa0: return 2;
	case 0xb0: return 3;
	case 0xb4: return 4;
	case 0xc0: return 5;
	case 0xc4: return 6;
	case 0xc8: return 7;
	case 0xd0: return 8;
	case 0xd4: return 9;
	default:
		fmt::print("[DMAC] Invalid channel id provided {:#x}, aborting...\n", value);
		std::abort();
	}
}

constexpr const char* REGS[] =
{
	"Dn_CHCR",
	"Dn_MADR",
	"Dn_QWC",
	"Dn_TADR",
	"Dn_ASR0",
	"Dn_ASR1",
	"", "",
	"Dn_SADR"
};

constexpr const char* GLOBALS[] =
{
	"D_CTRL",
	"D_STAT",
	"D_PCR",
	"D_SQWC",
	"D_RBSR",
	"D_RBOR",
	"D_STADT",
};

namespace ee
{
	DMAController::DMAController(common::Emulator* parent) :
		emulator(parent)
	{
		constexpr uint32_t addresses[] =
		{ 0x10008000, 0x10009000, 0x1000a000, 0x1000b000, 0x1000b400, 
		  0x1000c000, 0x1000c400, 0x1000c800, 0x1000d000, 0x1000d400 };

		/* Register channel handlers */
		for (auto addr : addresses)
		{
			emulator->add_handler(addr, this, &DMAController::read_channel, &DMAController::write_channel);
			/* Sadly the last register eats up a whole page itself */
			emulator->add_handler(addr + 0x80, this, &DMAController::read_channel, &DMAController::write_channel);
		}

		/* Register global register handlers */
		emulator->add_handler(0x1000E000, this, &DMAController::read_global, &DMAController::write_global);
	}

	uint32_t DMAController::read_channel(uint32_t addr)
	{
		assert((addr & 0xff) <= 0x80);

		uint32_t id = (addr >> 8) & 0xff;
		uint32_t channel = get_channel(id);
		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&channels[channel] + offset;

		fmt::print("[DMAC] Reading {:#x} from {} of channel {:d}\n", *ptr, REGS[offset], channel);
		return *ptr;
	}

	void DMAController::write_channel(uint32_t addr, uint32_t data)
	{
		assert((addr & 0xff) <= 0x80);

		uint32_t id = (addr >> 8) & 0xff;
		uint32_t channel = get_channel(id);
		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&channels[channel] + offset;

		fmt::print("[DMAC] Writing {:#x} to {} of channel {:d}\n", data, REGS[offset], channel);
		*ptr = data;
	}

	uint32_t DMAController::read_global(uint32_t addr)
	{
		assert(addr <= 0x1000E060);

		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&globals + offset;

		fmt::print("[DMAC] Reading {:#x} from {}\n", *ptr, GLOBALS[offset]);
		return *ptr;
	}

	void DMAController::write_global(uint32_t addr, uint32_t data)
	{
		assert(addr <= 0x1000E060);

		uint32_t offset = (addr >> 4) & 0xf;
		auto ptr = (uint32_t*)&globals + offset;

		fmt::print("[DMAC] Writing {:#x} to {}\n", data, GLOBALS[offset]);

		if (offset == 1) /* D_STAT */
		{
			/* The lower bits are cleared while the upper ones are reversed */
			globals.d_stat.clear &= ~(data & 0xffff);
			globals.d_stat.reverse ^= (data >> 16);
		}
		else
		{
			*ptr = data;
		}
	}
}