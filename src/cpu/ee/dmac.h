#pragma once
#include <common/component.h>

namespace common
{
	class Emulator;
}

namespace ee
{
	/* Have some common DMA tag ids */
	constexpr uint32_t DMATAG_END = 0x7;

	union TagAddr
	{
		uint32_t value;
		struct
		{
			uint32_t address : 30;
			uint32_t mem_select : 1;
		};
	};

	union DMATag
	{
		uint128_t value;
		struct
		{
			uint128_t qwords : 16;
			uint128_t : 11;
			uint128_t priority : 2;
			uint128_t id : 2;
			uint128_t irq : 1;
			uint128_t address : 31;
			uint128_t mem_select : 1;
			uint128_t data : 64;
		};
	};

	union DCHCR
	{
		uint32_t value;
		struct
		{
			uint32_t direction : 1; /* 0 = to memory, 1 = from memory */
			uint32_t : 1;
			uint32_t mode : 2; /* 0 = normal, 1 = chain, 2 = interleave */
			uint32_t stack_ptr : 2;
			uint32_t transfer_tag : 1;
			uint32_t enable_irq_bit : 1;
			uint32_t running : 1;
			uint32_t : 7;
			uint32_t tag : 16;
		};
	};

	struct DMACChannel
	{
		DCHCR control;
		uint32_t address;
		uint32_t qword_count;
		TagAddr tag_address;
		TagAddr saved_tag_address[2];
		uint32_t padding[2];
		uint32_t scratchpad_address;

		/* For use by emulator */
		bool end_transfer = false;
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
		uint32_t d_enable = 0x1201;
	};

	enum DMAChannels : uint32_t
	{
		VIF0,
		VIF1,
		GIF,
		IPU_FROM,
		IPU_TO,
		SIF0, /* from IOP */
		SIF1, /* to IOP */
		SIF2,
		SPR_FROM,
		SPR_TO
	};

	enum DMASourceID : uint32_t
	{
		REFE,
		CNT,
		NEXT,
		REF,
		REFS,
		CALL,
		RET,
		END
	};

	struct DMAController : public common::Component
	{
		DMAController(common::Emulator* parent);
		~DMAController() = default;

		/* I/O communication */
		uint32_t read_channel(uint32_t addr);
		uint32_t read_global(uint32_t addr);
		uint32_t read_enabler(uint32_t addr);

		void write_channel(uint32_t addr, uint32_t data);
		void write_global(uint32_t addr, uint32_t data);
		void write_enabler(uint32_t addr, uint32_t data);

		void tick(uint32_t cycles);

	private:
		void fetch_tag(uint32_t id);

	private:
		common::Emulator* emulator;

		/* Channels / Global registers */
		DMACChannel channels[10] = {};
		DMACGlobals globals = {};
	};
}