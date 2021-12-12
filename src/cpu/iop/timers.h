#pragma once
#include <cstdint>

namespace iop
{
	union TimerMode 
	{
		uint64_t value;
		struct 
		{
			uint64_t gate_enable : 1;
			uint64_t gate_mode : 2;
			uint64_t reset_on_intr : 1;
			uint64_t compare_intr : 1;
			uint64_t overflow_intr : 1;
			uint64_t repeat_intr : 1; /* if unset, bit 10 is set to 0 after interrupt occurs. */
			uint64_t levl : 1;
			uint64_t external_signal : 1;
			uint64_t tm2_prescaler : 1;
			uint64_t intr_enabled : 1;
			uint64_t compare_intr_raised : 1;
			uint64_t overflow_intr_raised : 1;
			uint64_t tm4_prescalar : 1;
			uint64_t tm5_prescalar : 1;
			uint64_t : 49;
		};
	};

	struct Timer
	{
		uint64_t count;
		TimerMode mode;
		uint64_t target;
	};

	class IOProcessor;
	class Timers {
	public:
		Timers(IOProcessor* iop);
		~Timers() = default;

		/* Add cycles to the timer. */
		void tick(uint32_t cycles);

		/* Read/Write to the timer. */
		uint32_t read(uint32_t address);
		void write(uint32_t address, uint32_t data);

	public:
		Timer timers[6] = {};
		IOProcessor* iop;
	};
}