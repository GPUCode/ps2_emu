#include <common/component.h>

namespace common
{
	class Emulator;
}

namespace spu
{
	/* This is temporary */
	struct SPU : public common::Component
	{
		SPU(common::Emulator* parent);
		uint32_t get_status(uint32_t address);
		void set_status(uint32_t address, uint32_t value);
		
		void trigger_irq();

	private:
		common::Emulator* emulator = nullptr;
		uint16_t status = 0;
	};
}