#include <cpu/ee/timers.h>
#include <cpu/ee/intc.h>
#include <common/emulator.h>
#include <fmt/core.h>
#include <cassert>

static const char* REGS[] =
{
	"TN_COUNT",
	"TN_MODE",
	"TN_COMP",
	"TN_HOLD"
};

static const char* CLOCK[] =
{
	"BUSCLK",
	"BUSCLK / 16",
	"BUSCLK / 256",
	"HBLANK"
};

namespace ee
{
	Timers::Timers(common::Emulator* emulator, INTC* intc) :
		intc(intc)
	{
		constexpr uint32_t addresses[] = { 0x10000000, 0x10000800, 0x10001000, 0x10001800 };
		for (auto addr : addresses)
		{
			emulator->add_handler(addr, this, &Timers::read, &Timers::write);
		}
	}

	uint32_t Timers::read(uint32_t addr)
	{
		/* Make sure the addresses are correct */
		assert((addr & 0xff) <= 0x30);

		int num = (addr & 0xff00) >> 11;
		uint32_t offset = (addr & 0xf0) >> 4;
		auto ptr = (uint32_t*)&timers[num] + offset;

		fmt::print("[TIMERS] Reading {:#x} from {} of timer {:d}\n", *ptr, REGS[offset], num);
		return *ptr;
	}

	void Timers::write(uint32_t addr, uint32_t data)
	{
		/* Make sure the addresses are correct */
		assert((addr & 0xff) <= 0x30);

		int num = (addr & 0xff00) >> 11;
		uint32_t offset = (addr & 0xf0) >> 4;
		auto ptr = (uint32_t*)&timers[num] + offset;

		fmt::print("[TIMERS] Writing {:#x} to {} of timer {:d}\n", data, REGS[offset], num);
		*ptr = data;

		/* Writes to Tn_MODE */
		if (offset == 1)
		{
			auto& timer = timers[num];
			
			/* Set the timer ratio based on the new clock 
			   0=Bus clock (~147 MHz)
			   1=Bus clock / 16
			   2=Bus clock / 256
			   3=HBLANK */
			switch (timer.mode.clock)
			{
			case 0:
				timer.ratio = EE_CLOCK / BUS_CLOCK; break;
			case 1:
				timer.ratio = EE_CLOCK / (BUS_CLOCK / 16); break;
			case 2:
				timer.ratio = EE_CLOCK / (BUS_CLOCK / 256); break;
			case 3: /* For now NTSC all the way... */
				timer.ratio = EE_CLOCK / HBLANK_NTSC; break;
			}

			fmt::print("[TIMERS] Setting timer {:d} clock to {}\n", num, CLOCK[timer.mode.clock]);
		}
	}

	void Timers::tick()
	{
		for (uint32_t i = 0; i < 3; i++)
		{
			auto& timer = timers[i];

			if (!timer.mode.enable)
				continue;

			/* Increment internal counter according to ratio */
			timer.counter = (timer.counter + 1) % (timer.ratio + 1);
			timer.count += (timer.counter == timer.ratio);

			/* Target reached */
			if (timer.count == timer.comp)
			{
				/* Timer interrupts are edge-triggered: an interrupt will only be 
				   sent to the EE if either interrupt flag goes from 0 to 1 */
				if (timer.mode.cmp_intr_enable && !timer.mode.cmp_flag)
				{
					intc->trigger(Interrupt::INT_TIMER0 + i);
				}

				/* Clear counter when it reaches target */
				if (timer.mode.clear_when_cmp)
				{
					timer.count = 0;
				}

				timer.mode.cmp_flag = 1;
			}

			/* Overflow check */
			if (timer.count > 0xffff)
			{
				if (timer.mode.overflow_intr_enable && !timer.mode.overflow_flag)
				{
					intc->trigger(Interrupt::INT_TIMER0 + i);
				}

				timer.count = 0;
				timer.mode.overflow_flag = 1;
			}
		}
	}
}