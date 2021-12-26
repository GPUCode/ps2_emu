#include <cpu/iop/timers.h>
#include <cpu/iop/iop.h>
#include <fmt/color.h>

namespace iop
{
	Timers::Timers(IOProcessor* iop) :
		iop(iop)
	{
	}
	
	void Timers::tick()
	{
		auto& timer = timers[5];

		timer.count++;
		if (timer.count == timer.target)
		{
			timer.mode.compare_intr_raised = true;
			if (timer.mode.compare_intr)
			{
				iop->intr.trigger(Interrupt::Timer5);
			}

			if (timer.mode.reset_on_intr)
			{
				timer.count = 0;
			}
		}

		if (timer.count > 0xFFFFFFFF)
		{
			timer.mode.overflow_intr_raised = true;
			if (timer.mode.overflow_intr)
			{
				iop->intr.trigger(Interrupt::Timer5);
			}
		}
	}
	
	uint32_t Timers::read(uint32_t address)
	{
		bool group = address & 0x400;
		uint32_t timer = (address & 0x30) >> 4;
		uint32_t offset = (address & 0xf) >> 2;

		auto ptr = (uint64_t*)&timers[timer + 3 * group] + offset;
		if (offset == 1) /* Reads from mode clear the two raised interrupt flags */
		{
			TimerMode& mode = timers[timer].mode;
			mode.compare_intr_raised = false;
			mode.overflow_intr_raised = false;
		}

		fmt::print("[IOP TIMERS] Reading {:#x} from timer {:d} at offset {:#x}\n", *ptr, timer + 3 * group, offset << 2);

		return *ptr;
	}
	
	void Timers::write(uint32_t address, uint32_t data)
	{
		bool group = address & 0x400;
		uint32_t timer = (address & 0x30) >> 4;
		uint32_t offset = (address & 0xf) >> 2;

		fmt::print("[IOP TIMERS] Writing {:#x} to timer {:d} at offset {:#x}\n", data, timer + 3 * group, offset << 2);

		auto ptr = (uint64_t*)&timers[timer + 3 * group] + offset;
		if (offset == 1) /* Writes to mode reset the count to zero and set bit 10 to 1 */
		{
			TimerMode& mode = timers[timer].mode;
			mode.intr_enabled = true;
			timers[timer].count = 0;
		}

		*ptr = data;
	}
}