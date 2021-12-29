#pragma once
#include <common/component.h>

namespace common
{
	class Emulator;
}

namespace ee
{
	union TagAddr
	{
		uint32_t value;
		struct
		{
			uint32_t address : 30;
			uint32_t mem_select : 1;
		};
	};

	struct DMACChannel
	{
		uint32_t control;
		uint32_t address;
		uint32_t qword_count;
		TagAddr tag_address;
		TagAddr saved_tag_address[2];
		uint32_t padding[2];
		uint32_t scratchpad_address;
	};

	/* Writing to this is a pain */
	union DSTAT
	{
		uint32_t value;
		struct
		{
			uint32_t channel_irq : 10; /* Clear with 1 */
			uint32_t : 3;
			uint32_t dma_stall : 1; /* Clear with 1 */
			uint32_t mfifo_empty : 1; /* Clear with 1 */
			uint32_t bus_error : 1; /* Clear with 1 */
			uint32_t channel_irq_mask : 10; /* Reverse with 1 */
			uint32_t : 3;
			uint32_t stall_irq_mask : 1; /* Reverse with 1 */
			uint32_t mfifo_irq_mask : 1; /* Reverse with 1 */
			uint32_t : 1;
		};
		/* If you notice above the lower 16bits are cleared when 1 is written to them
		   while the upper 16bits are reversed. So I'm making this struct to better
		   implement this behaviour */
		struct
		{
			uint32_t clear : 16;
			uint32_t reverse : 16;
		};
	};

	struct DMACGlobals
	{
		uint32_t d_ctrl;
		DSTAT d_stat;
		uint32_t d_pcr;
		uint32_t d_sqwc;
		uint32_t d_rbsr;
		uint32_t d_rbor;
		uint32_t d_stadr;
	};

	struct DMAController : public common::Component
	{
		DMAController(common::Emulator* parent);
		~DMAController() = default;

		/* I/O communication */
		uint32_t read_channel(uint32_t addr);
		void write_channel(uint32_t addr, uint32_t data);

		uint32_t read_global(uint32_t addr);
		void write_global(uint32_t addr, uint32_t data);

	private:
		common::Emulator* emulator;

		/* Channels / Global registers */
		DMACChannel channels[10] = {};
		DMACGlobals globals = {};
	};
}