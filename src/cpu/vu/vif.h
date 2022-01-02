#pragma once
#include <common/component.h>

namespace common
{
	class Emulator;
}

namespace vu
{
	union VIFSTAT
	{
		uint32_t value = 0;
		struct
		{
			uint32_t vif_status : 2;
			uint32_t e_wait : 1;
			uint32_t waiting_for_gif : 1;
			uint32_t : 2;
			uint32_t mark : 1;
			uint32_t double_buffer_flag : 1;
			uint32_t stalled_after_stop : 1;
			uint32_t stalled_after_break : 1;
			uint32_t stalled_on_intr : 1;
			uint32_t intr_detected : 1;
			uint32_t dmatag_mismatch : 1;
			uint32_t invalid_cmd : 1;
			uint32_t : 9;
			uint32_t fifo_detection : 1;
			uint32_t fifo_count : 5;
		};
	};

	union VIFFBRST
	{
		uint32_t value = 0;
		struct
		{
			uint32_t reset : 1;
			uint32_t force_break : 1;
			uint32_t stop_vif : 1;
			uint32_t stall_cancel : 1;
		};
	};

	union VIFCYCLE
	{
		uint32_t value;
		struct
		{
			uint32_t cycle_length : 8;
			uint32_t write_cycle_length : 8;
			uint32_t : 16;
		};
	};

	struct VIFRegs
	{
		VIFSTAT status;
		VIFFBRST fbrst;
		uint32_t err;
		uint32_t mark;
		VIFCYCLE cycle;
		uint32_t mode;
		uint32_t num;
		uint32_t mask;
		uint32_t code;
		uint32_t itops;
		/* These only on exist on VIF1,
		   but I add them as padding on VIF0 */
		uint32_t base;
		uint32_t ofst;
		uint32_t tops;
		uint32_t itop;
		uint32_t top;
		uint32_t rn[4];
		uint32_t cn[4];
	};

	/* Technically there are two VIFs (VIF0/VIF1) but 
	   they share 90% of their functionality so I'm
	   merging them in a single class */
	struct VIF : public common::Component
	{
		VIF(common::Emulator* parent);
		~VIF() = default;

		uint32_t read(uint32_t addr);
		void write(uint32_t addr, uint32_t data);

		void write_fifo(uint32_t addr, uint128_t data);

	private:
		common::Emulator* emulator;
		VIFRegs registers[2] = {};
	};
}