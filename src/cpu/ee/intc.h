#pragma once
#include <cstdint>

namespace ee
{
	enum Interrupt : uint32_t
	{
		INT_GS,
		INT_SBUS,
		INT_VB_ON,
		INT_VB_OFF,
		INT_VIF0,
		INT_VIF1,
		INT_VU0,
		INT_VU1,
		INT_IPU,
		INT_TIMER0,
		INT_TIMER1,
		INT_TIMER2,
		INT_TIMER3,
		INT_SFIFO,
		INT_VU0WD
	};

	/* INTC */
	struct INTCRegs
	{
		uint32_t intc_stat;
		uint32_t intc_mask;
	};

    struct EmotionEngine;
	class INTC
	{
	public:
		INTC(EmotionEngine* ee);
		~INTC() = default;

		uint64_t read(uint32_t addr);
		void write(uint32_t addr, uint64_t data);

		void trigger(uint32_t intr);
		bool int_pending();

	private:
		EmotionEngine* cpu;
		INTCRegs regs;
	};
}
