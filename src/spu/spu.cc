#include <spu/spu.h>
#include <common/emulator.h>
#include <cassert>

namespace spu
{
	SPU::SPU(common::Emulator* parent) :
		emulator(parent)
	{
		emulator->add_handler(0x1F900744, this, &SPU::get_status, &SPU::set_status);
	}

	uint32_t SPU::get_status(uint32_t address)
	{
		if (address == 0x1F900744)
		{
			uint16_t copy = status;
			status &= ~0x80;
			return copy;
		}

		return 0;
	}

	void SPU::set_status(uint32_t address, uint32_t value)
	{

	}

	void SPU::trigger_irq()
	{
		status |= 0x80;
	}
}