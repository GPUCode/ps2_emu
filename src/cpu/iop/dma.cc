#include <cpu/iop/dma.h>
#include <common/emulator.h>
#include <fmt/color.h>
#include <cassert>

constexpr const char* REGS[] =
{
	"Dn_MADR", "Dn_BCR",
	"Dn_CHCR", "Dn_TADR"
};

constexpr const char* GLOBALS[] =
{
	"DPCR", "DICR", "DPCR2",
	"DICR2", "DMACEN", "DMACINTEN"
};

namespace iop
{
	/* DMA Controller class implementation. */
	DMAController::DMAController(common::Emulator* emu) :
		emulator(emu)
	{
		/* Register our functions to the Emulator. */
		emulator->add_handler(0x1f801080, this, &DMAController::read, &DMAController::write);
		emulator->add_handler(0x1f801500, this, &DMAController::read, &DMAController::write);
	}

	void DMAController::tick()
	{
	}

	void DMAController::start(uint32_t channel)
	{

	}

	uint32_t DMAController::read(uint32_t address)
	{
		uint16_t group = (address >> 8) & 0x1;

		/* Read globals */
		if ((address & 0x70) == 0x70)
		{
			uint16_t offset = ((address & 0xf) >> 2) + 2 * group;
			auto ptr = (uint32_t*)&globals + offset;
			fmt::print("[IOP DMA] Reading {:#x} from global register {}\n", *ptr, GLOBALS[offset]);
			return *ptr;
		}
		else /* Read from channel */
		{
			uint16_t channel = ((address & 0x70) >> 4) + group * 7;
			uint16_t offset = (address & 0xf) >> 2;
			auto ptr = (uint32_t*)&channels[channel] + offset;
			fmt::print("[IOP DMA] Reading {:#x} from {} in channel {:d}\n", *ptr, REGS[offset], channel);
			return *ptr;
		}
	}

	void DMAController::write(uint32_t address, uint32_t data)
	{
		/* Get channel information from address. */
		uint16_t group = (address >> 8) & 0x1;

		/* Write globals */
		if ((address & 0x70) == 0x70)
		{
			uint16_t offset = ((address & 0xf) >> 2) + 2 * group;
			auto ptr = (uint32_t*)&globals + offset;
			fmt::print("[IOP DMA] Writing {:#x} to {}\n", data, GLOBALS[offset]);

			*ptr = data;

			/* Handle special case for writing to DICR */
			if (offset == 1)
			{
				auto& irq = globals.dicr;
				irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));
			}
		}
		else /* Write channels */
		{
			uint16_t channel = ((address & 0x70) >> 4) + group * 7;
			uint16_t offset = (address & 0xf) >> 2;
			auto ptr = (uint32_t*)&channels[channel] + offset;
			fmt::print("[IOP DMA] Writing {:#x} to {} of channel {:d}\n", data, REGS[offset], channel);

			*ptr = data;
		}
	}
}