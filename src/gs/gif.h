#pragma once
#include <common/component.h>
#include <utils/queue.h>

namespace common
{
	class Emulator;
}

namespace gs
{
	union GIFCTRL
	{
		uint32_t value;
		struct
		{
			uint32_t reset : 1;
			uint32_t : 2;
			uint32_t stop : 1;
		};
	};

	union GIFSTAT
	{
		uint32_t value;
		struct
		{
			uint32_t path3_mask : 1;
			uint32_t path3_vif_mask : 1;
			uint32_t path3_imt : 1;
			uint32_t stop : 1;
			uint32_t : 1;
			uint32_t path3_interrupted : 1;
			uint32_t path3_queued : 1;
			uint32_t path2_queued : 1;
			uint32_t path1_queued : 1;
			uint32_t output_path : 1;
			uint32_t active_path : 2;
			uint32_t direction : 1;
			uint32_t : 12;
			uint32_t fifo_count : 5;
		};
	};

	/* The header of each GS Primitive */
	union GIFTag
	{
		uint128_t value;
		struct
		{
			uint128_t nloop : 15;
			uint128_t eop : 1;
			uint128_t : 30;
			uint128_t pre : 1;
			uint128_t prim : 11;
			uint128_t flg : 2;
			uint128_t nreg : 4;
			uint128_t regs : 64;
		};
	};

	enum REGDesc : uint64_t
	{
		PRIM = 0,
		RGBAQ = 1,
		ST = 2,
		UV = 3,
		XYZF2 = 4, 
		XYZ2 = 5,
		TEX0_1 = 6,
		TEX0_2 = 7,
		CLAMP_1 = 8,
		CLAMP_2 = 9,
		FOG = 10,
		XYZF3 = 12,
		XYZ3 = 13,
		A_D = 14,
		NOP = 15
	};

	enum Format : uint32_t
	{
		Packed = 0,
		Reglist = 1,
		Image = 2,
		Disable = 3
	};

	struct GIF : public common::Component
	{
		GIF(common::Emulator* parent);
		~GIF() = default;

		void tick(uint32_t cycles);
		void reset();

		uint32_t read(uint32_t addr);
		void write(uint32_t addr, uint32_t data);

		/* Writes from various PATHs */
		bool write_path3(uint32_t, uint128_t data);

	private:
		void process_tag();
		void execute_command();

		void process_packed(uint128_t qword);

	private:
		common::Emulator* emulator;
		
		/* GIF registers */
		GIFCTRL control = {};
		uint32_t mode = 0;
		GIFSTAT status = {};
		util::Queue<uint32_t, 64> fifo;

		/* Used during transfers */
		GIFTag tag = {};
		int data_count = 0, reg_count = 0;

		/* Stores the last Q value provided by ST */
		uint32_t interal_Q;
	};
}