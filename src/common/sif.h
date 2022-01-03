#pragma once
#include <common/component.h>

namespace common
{
	struct SIFRegs
	{
		uint32_t mscom;
		uint32_t smcom;
		uint32_t msflg;
		uint32_t smflg;
		uint32_t ctrl;
		uint32_t padding;
		uint32_t bd6;
	};

	class Emulator;
	struct SIF : public common::Component
	{
		SIF(Emulator* parent);

		uint32_t read(uint32_t addr);
		void write(uint32_t addr, uint32_t data);

	private:
		Emulator* emulator;
		SIFRegs regs = {};
	};
}