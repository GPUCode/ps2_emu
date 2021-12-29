#pragma once
#include <common/component.h>

namespace common
{
	class Emulator;
}

namespace ee
{
	/* Timer contants from ps2sdk */
	constexpr uint32_t EE_CLOCK = 294912000;
	constexpr uint32_t BUS_CLOCK = EE_CLOCK / 2;
	constexpr uint32_t HBLANK_NTSC = 15734;
	constexpr uint32_t HBLANK_PAL = 15625;

	union TnMode
	{
		uint32_t value;
		struct
		{
			uint32_t clock : 2;
			uint32_t gate_enable : 1;
			uint32_t gate_type : 1;
			uint32_t gate_mode : 2;
			uint32_t clear_when_cmp : 1;
			uint32_t enable : 1;
			uint32_t cmp_intr_enable : 1;
			uint32_t overflow_intr_enable : 1;
			uint32_t cmp_flag : 1;
			uint32_t overflow_flag : 1;
			uint32_t : 20;
		};
	};

	struct Timer
	{
		uint32_t count;
		TnMode mode;
		uint32_t comp;
		uint32_t hold; /* Only exists for T0 and T1 */

		/* For use by the emulator */
		uint32_t counter;
		/* The variable here means that the timer will increment every *ratio* EE cycles.
		   Since the EE has the fastest frequency I choose it use it as a comparison metric. */
		uint32_t ratio;
	};

	class INTC;
	struct Timers : public common::Component
	{
		Timers(common::Emulator* emulator, INTC* intc);
		~Timers() = default;

		void tick();

		uint32_t read(uint32_t addr);
		void write(uint32_t addr, uint32_t data);

	private:
		INTC* intc;
		Timer timers[4] = {};
	};
}